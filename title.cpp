#include "title.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "shader.h"
#include "direct3d.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "ClickFont.h"
#include <mfapi.h>
#include <mfmediaengine.h>
#include <mfidl.h>
#include <d3d10.h>
#include <string>
#include <cmath>
#include "WinAnim.h"
using namespace DirectX;

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

template<class T>
static void SafeReleaseCOM(T*& p)
{
	if (p)
	{
		p->Release();
		p = nullptr;
	}
}

class TitleMediaEngineNotify : public IMFMediaEngineNotify
{
public:
	TitleMediaEngineNotify() : m_refCount(1), m_canPlay(false), m_pEngine(nullptr) {}

	void SetEngine(IMFMediaEngine* pEngine)
	{
		m_pEngine = pEngine;
	}

	bool CanPlay() const
	{
		return m_canPlay;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
	{
		if (!ppv) return E_POINTER;
		if (riid == IID_IUnknown || riid == __uuidof(IMFMediaEngineNotify))
		{
			*ppv = static_cast<IMFMediaEngineNotify*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef() override
	{
		return (ULONG)InterlockedIncrement(&m_refCount);
	}

	STDMETHODIMP_(ULONG) Release() override
	{
		ULONG count = (ULONG)InterlockedDecrement(&m_refCount);
		if (count == 0)
		{
			delete this;
		}
		return count;
	}

	STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR, DWORD) override
	{
		if (meEvent == MF_MEDIA_ENGINE_EVENT_CANPLAY)
		{
			m_canPlay = true;
			if (m_pEngine)
			{
				m_pEngine->Play();
			}
		}
		return S_OK;
	}

private:
	~TitleMediaEngineNotify() = default;
	LONG m_refCount;
	bool m_canPlay;
	IMFMediaEngine* m_pEngine;
};

static IMFDXGIDeviceManager* g_pDXGIDeviceManager = nullptr;
static IMFMediaEngine* g_pMediaEngine = nullptr;
static TitleMediaEngineNotify* g_pMediaEngineNotify = nullptr;
static ID3D11Texture2D* g_pVideoTexture = nullptr;
static ID3D11ShaderResourceView* g_pVideoSRV = nullptr;
static UINT g_DxgiResetToken = 0;
static void FinalizeTitleMovie();

static std::wstring MakeFileUrl(const wchar_t* relativePath)
{
	wchar_t fullPath[MAX_PATH] = {};
	if (!GetFullPathNameW(relativePath, MAX_PATH, fullPath, nullptr))
	{
		return L"";
	}

	std::wstring url = L"file:///";

	for (wchar_t ch : std::wstring(fullPath))
	{
		url.push_back((ch == L'\\') ? L'/' : ch);
	}

	return url;
}

static bool InitializeTitleMovie()
{
	ID3D11Device* pDevice = Direct3D_GetDevice();
	if (!pDevice) return false;
	FinalizeTitleMovie();

	// D3D11デバイスのマルチスレッド保護を有効化（IMFMediaEngineはバックグラウンドスレッドからアクセスする）
	ID3D10Multithread* pMultithread = nullptr;
	HRESULT hr = pDevice->QueryInterface(__uuidof(ID3D10Multithread), (void**)&pMultithread);
	if (SUCCEEDED(hr) && pMultithread)
	{
		pMultithread->SetMultithreadProtected(TRUE);
		pMultithread->Release();
	}

	// ビデオテクスチャは動画の解像度に合わせる（3840x2160では大きすぎる）
	const UINT videoTexW = 1920;
	const UINT videoTexH = 1080;

	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = videoTexW;
	texDesc.Height = videoTexH;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	hr = pDevice->CreateTexture2D(&texDesc, nullptr, &g_pVideoTexture);
	if (FAILED(hr)) {
		FinalizeTitleMovie();
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	hr = pDevice->CreateShaderResourceView(g_pVideoTexture, &srvDesc, &g_pVideoSRV);
	if (FAILED(hr)) {
		FinalizeTitleMovie();
		return false;
	}

	hr = MFCreateDXGIDeviceManager(&g_DxgiResetToken, &g_pDXGIDeviceManager);
	if (FAILED(hr)) {
		FinalizeTitleMovie();
		return false;
	}

	hr = g_pDXGIDeviceManager->ResetDevice(pDevice, g_DxgiResetToken);
	if (FAILED(hr)) {
		FinalizeTitleMovie();
		return false;
	}

	g_pMediaEngineNotify = new TitleMediaEngineNotify();
	if (!g_pMediaEngineNotify) {
		FinalizeTitleMovie();
		return false;
	}

	IMFAttributes* pAttributes = nullptr;
	IMFMediaEngineClassFactory* pFactory = nullptr;

	hr = MFCreateAttributes(&pAttributes, 4);
	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, g_pDXGIDeviceManager);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, g_pMediaEngineNotify);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);
	}
	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pFactory));
	}
	if (SUCCEEDED(hr))
	{
		hr = pFactory->CreateInstance(MF_MEDIA_ENGINE_REAL_TIME_MODE, pAttributes, &g_pMediaEngine);
	}

	SafeReleaseCOM(pFactory);
	SafeReleaseCOM(pAttributes);

	if (FAILED(hr) || !g_pMediaEngine)
	{
		FinalizeTitleMovie();
		return false;
	}

	g_pMediaEngineNotify->SetEngine(g_pMediaEngine);
	g_pMediaEngine->SetLoop(TRUE);
	g_pMediaEngine->SetMuted(TRUE);

	std::wstring videoUrl = MakeFileUrl(L"asset\\texture\\titlemov.mp4");
	if (videoUrl.empty()) {
		FinalizeTitleMovie();
		return false;
	}

	BSTR bstrUrl = SysAllocString(videoUrl.c_str());
	if (!bstrUrl) {
		FinalizeTitleMovie();
		return false;
	}

	g_pMediaEngine->SetSource(bstrUrl);
	SysFreeString(bstrUrl);
	g_pMediaEngine->Load();

	return true;
}

static void UpdateTitleMovie()
{
	if (!g_pMediaEngine || !g_pVideoTexture) return;

	// ビデオストリームが存在し、十分に読み込まれていることを確認
	if (!g_pMediaEngine->HasVideo()) return;

	if (g_pMediaEngine->GetReadyState() < MF_MEDIA_ENGINE_READY_HAVE_CURRENT_DATA) return;

	LONGLONG pts = 0;
	HRESULT hr = g_pMediaEngine->OnVideoStreamTick(&pts);
	if (hr == S_OK)
	{
		// テクスチャの実際のサイズに合わせて転送先RECTを設定
		D3D11_TEXTURE2D_DESC desc{};
		g_pVideoTexture->GetDesc(&desc);
		RECT dst = { 0, 0, (LONG)desc.Width, (LONG)desc.Height };
		MFARGB border = { 255, 0, 0, 0 };
		hr = g_pMediaEngine->TransferVideoFrame(g_pVideoTexture, nullptr, &dst, &border);
		if (FAILED(hr))
		{
			// TransferVideoFrame失敗時は描画をスキップできるようSRVをnullに
			SafeReleaseCOM(g_pVideoSRV);
		}
	}
}

static void FinalizeTitleMovie()
{
	// コールバックからのエンジン参照を先に解除（バックグラウンドスレッドからのアクセスを防ぐ）
	if (g_pMediaEngineNotify)
	{
		g_pMediaEngineNotify->SetEngine(nullptr);
	}

	if (g_pMediaEngine)
	{
		g_pMediaEngine->Shutdown();
	}

	SafeReleaseCOM(g_pMediaEngine);
	SafeReleaseCOM(g_pMediaEngineNotify);
	SafeReleaseCOM(g_pDXGIDeviceManager);
	SafeReleaseCOM(g_pVideoSRV);
	SafeReleaseCOM(g_pVideoTexture);
	g_DxgiResetToken = 0;
}

// ①Spriteのインスタンス、ポインタ用意
static SplitSprite* g_pTitleSprite = nullptr;
static FontRenderer* g_pTitleFont2 = nullptr;
static ClickFont* g_pStartClickFont = nullptr;
static ClickFont* g_pModelViewerClickFont = nullptr;
static Sprite* g_pSizeComparisonSprite = nullptr;
static Sprite* g_pinazuma = nullptr;
static SoundData* g_pBGM = nullptr;

// 稲妻関連（opanim.cppから移植）
static Sprite* g_pInazumaSprite = nullptr;

static struct {
	float timer;
	float nextTrigger;
	bool active;
	float flashAlpha;
	unsigned int seed;
} g_Inazuma = { 0.0f, 0.2f, false, 0.0f, 0xC0FFEEu };

static const float INAZUMA_FLASH_DURATION_MIN = 0.08f;
static const float INAZUMA_FLASH_DURATION_MAX = 0.2f;
static const float INAZUMA_INTERVAL_MIN = 0.5f;
static const float INAZUMA_INTERVAL_MAX = 3.5f;
static const float INAZUMA_BASE_ALPHA = 0.5f;

// タイトルロゴ瞬き関連
static struct {
	float blinkTimer;
	float blinkCycle;
} g_LogoBlink = { 0.0f, 1.5f };	// 1.5秒周期で瞬く

// ========================
// ユーティリティ関数
// ========================

// 線形合同法による乱数生成 [0.0, 1.0)
static float Rand01()
{
	g_Inazuma.seed = g_Inazuma.seed * 1664525u + 1013904223u;
	return (float)(g_Inazuma.seed & 0x00FFFFFFu) / (float)0x01000000u;
}

// イージング関数（ease-out cubic）
static float EaseOutCubic(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	float inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

// 値をクランプ
static float Clamp(float value, float min, float max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	InitializeTitleMovie();

	// ②各種初期化
	g_pTitleSprite = new SplitSprite(
		{ SCREEN_WIDTH / 2 - 200.0f, SCREEN_HEIGHT / 2.0f - 100.0f },		//位置
		{ SCREEN_WIDTH * 0.7f, SCREEN_HEIGHT * 0.7f },						//サイズ
		0.0f,															//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },											//RGBA
		BLENDSTATE_NONE,												//BlendState
		L"asset\\texture\\title.png",											//テクスチャパス
		2, 1															//分割数X, Y
	);

	//日本語フォント描画（クリック対応）
	g_pStartClickFont = new ClickFont(
		{ SCREEN_WIDTH / 4.0f - 20.0f, (SCREEN_HEIGHT - 170.0f) },
		100.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.85f, 0.2f, 1.0f },
		"Click to Start"
	);
	g_pStartClickFont->SetHitSize({ 1000.0f, 1000.0f });
	g_pStartClickFont->SetOnClick([]() {
		StartFade(SCENE_ANM_OP);
	});

	// モデルビューワシーンへのClickFont（左上）
	g_pModelViewerClickFont = new ClickFont(
		{ SCREEN_WIDTH - 130.0f, 20.0f },
		22.0f,
		0.0f,
		{ 0.7f, 0.7f, 0.7f, 1.0f },
		{ 0.3f, 0.9f, 1.0f, 1.0f },
		"[Debug] ModelViewer"
	);
	g_pModelViewerClickFont->SetHitSize({ 260.0f, 30.0f });
	g_pModelViewerClickFont->SetOnClick([]() {
		StartFade(SCENE_DEBUG_MODEL);
	});

	//日本語フォント描画
	g_pTitleFont2 = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },			//位置（画面中央）
		200.0f,												//フォントサイズ（ピクセル）
		0.0f,												//回転
		{ 0.0f, 0.8f, 0.8f, 0.8f },									//RGBA
		"g_pTitleFont2"													//テキスト
	);

	// サイズ比較用Sprite（1.png 32x32 中央配置）
	g_pSizeComparisonSprite = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },			//位置（画面中央）
		{ SCREEN_HEIGHT, SCREEN_HEIGHT },						//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },								//RGBA
		BLENDSTATE_ALFA,										//テキスト
		L"asset\\texture\\guide.png"
	);

	g_pinazuma = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },			//位置（画面中央）
		{ 1280,720 },						//フォントサイズ（ピクセル）
		0.0f,													//回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },								//RGBA
		BLENDSTATE_ALFA,										//テキスト
		L"asset\\yureihen\\inazuma2.png"
	);

	// 稲妻スプライト初期化
	g_pInazumaSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置（画面中央）
		{ 1280.0f, 720.0f },						// サイズ
		0.0f,										// 回転（度）
		{ 0.95f, 0.95f, 1.0f, INAZUMA_BASE_ALPHA },	// 色
		BLENDSTATE_ADD,								// BlendState（加算合成）
		L"asset\\yureihen\\inazuma2.png"		// テクスチャパス
	);

	// 稲妻初期化
	g_Inazuma.nextTrigger = 0.5f + Rand01() * 1.5f;
	g_Inazuma.active = false;

	// BGM再生
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) {
		PlaySound(g_pBGM, true);
	}

	// タイトル画面ではマウスカーソルを表示・絶対モードに設定
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
	// マウスロックを明示的に解除
	ShowCursor(TRUE);
}

void Title_Update(void)
{
	UpdateTitleMovie();

	// タイトルロゴ瞬きタイマー更新
	g_LogoBlink.blinkTimer += (1.0f / 60.0f);	// 60FPS想定
	if (g_LogoBlink.blinkTimer >= g_LogoBlink.blinkCycle) {
		g_LogoBlink.blinkTimer = 0.0f;
	}

	// 瞬き周期内での時間比率 [0.0, 1.0)
	float blinkProgress = g_LogoBlink.blinkTimer / g_LogoBlink.blinkCycle;
	// 0.0～0.15: 消える、0.15～1.0: 現れる
	float logoAlpha = (blinkProgress < 0.15f) ? (1.0f - blinkProgress / 0.15f) : 1.0f;

	// 稲妻更新
	g_Inazuma.nextTrigger -= (1.0f / 60.0f);	// 60FPS想定
	if (g_Inazuma.nextTrigger <= 0.0f && !g_Inazuma.active)
	{
		g_Inazuma.active = true;
		g_Inazuma.flashAlpha = 0.7f + Rand01() * 0.3f;
		g_Inazuma.timer = 0.0f;
	}

	if (g_Inazuma.active)
	{
		g_Inazuma.timer += (1.0f / 60.0f);
		float duration = INAZUMA_FLASH_DURATION_MIN + Rand01() * (INAZUMA_FLASH_DURATION_MAX - INAZUMA_FLASH_DURATION_MIN);
		float fade = 1.0f - EaseOutCubic(g_Inazuma.timer / duration);
		g_Inazuma.flashAlpha *= fade;

		if (g_Inazuma.timer >= duration)
		{
			g_Inazuma.active = false;
			g_Inazuma.flashAlpha = 0.0f;
			g_Inazuma.nextTrigger = INAZUMA_INTERVAL_MIN + Rand01() * (INAZUMA_INTERVAL_MAX - INAZUMA_INTERVAL_MIN);
		}
	}

	// 稲妻アルファ更新
	float inazumaAlpha = INAZUMA_BASE_ALPHA + g_Inazuma.flashAlpha * 0.9f;
	if (g_pInazumaSprite) {
		g_pInazumaSprite->SetColor({ 0.95f, 0.95f, 1.0f, Clamp(inazumaAlpha, 0.0f, 1.5f) });
	}

	if (g_pStartClickFont) g_pStartClickFont->Update();
	if (g_pModelViewerClickFont) g_pModelViewerClickFont->Update();

	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		StartFade(SCENE_ANM_OP);
	}

	if (Keyboard_IsKeyDownTrigger(KK_L))
	{
		StartFade(SCENE_ANM_LOSE);
	}

	//// ③適当な処理　アニメーションなどもここで
	//if (Keyboard_IsKeyDownTrigger(KK_X))
	//{
	//	StartFade(SCENE_RESULT);
	//}
	if (Keyboard_IsKeyDownTrigger(KK_P))
	{
		if (Keyboard_IsKeyDownTrigger(KK_O))
		{		//3桁のスコアを適当にセット（Time=50, Combo=5 → Score=250）
			WinAnim_SetResultData(150.0f, 5);

			//Debug用
			StartFade(SCENE_ANM_WIN);
		}
	}

	//if (Keyboard_IsKeyDownTrigger(KK_F))
	//{
	//	//3桁のスコアを適当にセット（Time=50, Combo=5 → Score=250）
	//	WinAnim_SetResultData(350.0f, 5);

	//	//Debug用
	//	StartFade(SCENE_ANM_WIN);
	//}

}

void Title_Draw(void)
{
	if (g_pVideoSRV)
	{
		Sprite_Single_Draw(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
			{ SCREEN_WIDTH, SCREEN_HEIGHT },
			0.0f,
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			BLENDSTATE_NONE,
			g_pVideoSRV
		);
	}

	// タイトルロゴ瞬きアルファ値の計算
	float blinkProgress = g_LogoBlink.blinkTimer / g_LogoBlink.blinkCycle;
	float logoAlpha = (blinkProgress < 0.15f) ? (1.0f - blinkProgress / 0.15f) : 1.0f;

	// タイトルロゴスプライトへのアルファ値適用（親クラスのSetColor互換メソッドが存在する場合）
	// 参考：g_pinazumaと同様にSetColor経由でアルファ値を設定

	//g_pTitleSprite->Draw();
	//g_pSizeComparisonSprite->Draw();
	if (g_pStartClickFont) g_pStartClickFont->Draw();
	if (g_pModelViewerClickFont) g_pModelViewerClickFont->Draw();
	//g_pTitleFont2->Draw();
	//g_pinazuma->Draw();
	//if (g_pInazumaSprite) g_pInazumaSprite->Draw();	// 稲妻描画
}

void Title_Finalize(void)
{
	FinalizeTitleMovie();

	delete g_pTitleSprite;
	g_pTitleSprite = nullptr;

	delete g_pSizeComparisonSprite;
	g_pSizeComparisonSprite = nullptr;

	delete g_pTitleFont2;
	g_pTitleFont2 = nullptr;

	delete g_pinazuma;
	g_pinazuma = nullptr;

	delete g_pInazumaSprite;
	g_pInazumaSprite = nullptr;

	if (g_pStartClickFont) {
		delete g_pStartClickFont;
		g_pStartClickFont = nullptr;
	}
	if (g_pModelViewerClickFont) {
		delete g_pModelViewerClickFont;
		g_pModelViewerClickFont = nullptr;
	}

	// BGM解放
	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}
