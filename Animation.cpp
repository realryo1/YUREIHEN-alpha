#include "animation.h"
#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Logo Animation (ロゴアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
SplitSprite* g_LogoSprite = nullptr;
Sprite* g_BG = nullptr;

void Animation_Logo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	g_LogoSprite = new SplitSprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },// 位置
		{ 1000.0f, 1000.0f },					// サイズ
		0.0f,									// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,						// BlendState
		L"asset\\texture\\violisunlogo.png",	// テクスチャパス
		2, 1									// 分割数X, Y
	);

	g_BG = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },// 位置
		{ SCREEN_WIDTH, SCREEN_HEIGHT },		// サイズ
		0.0f,									// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,						// BlendState
		L"asset\\texture\\fade.png"			// テクスチャパス
	);
}

void Animation_Logo_Update(void)
{
	//スペースを押したらロゴを変える
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		//テクスチャが1ならタイトル画面へ
		if (g_LogoSprite->GetTextureNumber() == 1)
		{
			//タイトル画面へ移行する処理をここに追加
			StartFade(SCENE_TITLE);
		}
		else
		{
			StartFade();
		}
	}

	//ほかのシーンから戻ってくるとバグる
	if (GetFadeState() == FADE_IN)
	{
		g_LogoSprite->SetTextureNumber(1);
		g_BG->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
	}
}

void Animation_Logo_Draw(void)
{
	g_BG->Draw();
	g_LogoSprite->Draw();
}

void Animation_Logo_Finalize(void)
{
	delete g_LogoSprite;
	delete g_BG;
}

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Op Animation (Openingアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
void Animation_Op_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) {
		hal::dout << "Animation_Op_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	g_pDevice = pDevice;
	g_pContext = pContext;
}

void Animation_Op_Update(void)
{
}

void Animation_Op_Draw(void)
{
}

void Animation_Op_Finalize(void)
{
}

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Win Animation (勝ちアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
void Animation_Win_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) {
		hal::dout << "Animation_Win_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	g_pDevice = pDevice;
	g_pContext = pContext;
}

void Animation_Win_Update(void)
{
}

void Animation_Win_Draw(void)
{
}

void Animation_Win_Finalize(void)
{
}

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Lose Animation (負けアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
void Animation_Lose_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (!pDevice || !pContext) {
		hal::dout << "Animation_Lose_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	g_pDevice = pDevice;
	g_pContext = pContext;
}

void Animation_Lose_Update(void)
{
}

void Animation_Lose_Draw(void)
{
}

void Animation_Lose_Finalize(void)
{
}
