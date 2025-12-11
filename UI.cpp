#include "camera.h"
#include "UI.h"
#include "sprite.h"
#include "debug_ostream.h"
#include "keyboard.h"
#include "fade.h"
#include "UI_scarecombo.h"
#include "field.h"
#include "define.h"
#include "ghost.h"

// グローバル変数
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;
static DWORD g_LastScoreUpdateTime = 0;

static Number* g_FloorNumber = nullptr;
static Sprite* g_FloorTextF = nullptr;

// クリックガイド用
static Sprite* g_GuideClick = nullptr;

// 階層移動ガイド用
static Number* g_GuideFloorNum = nullptr;
static Sprite* g_GuideFloorF = nullptr;
static bool g_ShowGuideFloor = false;

// 各階層のゲージ値を保存する配列
static float g_FloorGaugeValues[MAP_FLOORS];
// 前フレームの階層を記憶しておく変数
static int g_LastFrameFloor = -1;


// 3D座標 -> 2Dスクリーン座標変換
static XMFLOAT2 WorldToScreen(const XMFLOAT3& worldPos)
{
	Camera* camera = GetCamera();
	// ★修正: 明示的なコンストラクタを使用
	if (!camera) return XMFLOAT2(-100.0f, -100.0f);

	XMMATRIX view = camera->GetView();
	XMMATRIX projection = camera->GetProjection();
	XMMATRIX viewProj = view * projection;

	XMVECTOR posVec = XMLoadFloat3(&worldPos);
	posVec = XMVectorSetW(posVec, 1.0f);

	XMVECTOR clipPos = XMVector3TransformCoord(posVec, viewProj);
	XMFLOAT3 ndc;
	XMStoreFloat3(&ndc, clipPos);

	// 画面外判定
	if (ndc.z < 0.0f || ndc.z > 1.0f)
	{
		return XMFLOAT2(-1000.0f, -1000.0f);
	}

	float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
	float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;

	return XMFLOAT2(screenX, screenY);
}

//----------------------------
//UI初期化
//----------------------------
void UI_Initialize(void)
{
	g_Clock = new Timer(
		{ CLOCK_POS_X, CLOCK_POS_Y },
		{ CLOCK_SIZE, CLOCK_SIZE },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\clock.png",
		2, 1,
		CLOCK_MIN, CLOCK_MAX
	);

	// クリックガイド
	g_GuideClick = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 100.0f },
		{ 100.0f, 100.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\click_guide.png"
	);

	// 階層移動ガイド(数字)
	g_GuideFloorNum = new Number(
		{ 0.0f, 0.0f },
		{ 40.0f, 40.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		25.0f
	);

	// 階層移動ガイド(F)
	g_GuideFloorF = new Sprite(
		{ 0.0f, 0.0f },
		{ 40.0f, 40.0f },
		0.0f,
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\floor_f.png"
	);

	g_ScareGauge = new Gauge(
		{ SCREEN_WIDTH - 270.0f, 70.0f },
		{ GAUGE_SIZE, GAUGE_SIZE },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\gauge.png",
		3, 1,
		0.0f, 100.0f,
		2, 0
	);

	g_Reticle = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ 30.0f, 30.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.5f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\grass.png"
	);

	UI_ScareCombo_Initialize();

	// 左上の階層表示
	float floorPosX = CLOCK_POS_X;
	float floorPosY = CLOCK_POS_Y + 130.0f;

	g_FloorNumber = new Number(
		{ floorPosX - 20.0f, floorPosY },
		{ 60.0f, 60.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		40.0f
	);
	g_FloorNumber->SetNumber(1);

	g_FloorTextF = new Sprite(
		{ floorPosX + 30.0f, floorPosY },
		{ 60.0f, 60.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\floor_f.png"
	);

	// ゲージ管理初期化
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		g_FloorGaugeValues[i] = 50.0f;
	}

	g_LastFrameFloor = Field_GetCurrentFloor();
	g_ScareGauge->SetValue(g_FloorGaugeValues[g_LastFrameFloor]);
}

//----------------------------
//UI更新
//----------------------------
void UI_Update(void)
{
	int currentFloor = Field_GetCurrentFloor();

	// --- 階層切り替え時のゲージ退避・復元 ---
	if (g_LastFrameFloor != currentFloor)
	{
		if (g_LastFrameFloor >= 0 && g_LastFrameFloor < MAP_FLOORS)
		{
			g_FloorGaugeValues[g_LastFrameFloor] = g_ScareGauge->GetValue();
		}

		if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
		{
			g_ScareGauge->SetValue(g_FloorGaugeValues[currentFloor]);
		}

		g_LastFrameFloor = currentFloor;
	}
	else
	{
		if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
		{
			g_FloorGaugeValues[currentFloor] = g_ScareGauge->GetValue();
		}
	}

	// --- 勝利判定 ---
	bool allCleared = true;
	float maxValue = g_ScareGauge->GetMaxValue();

	for (int i = 0; i < MAP_FLOORS; i++)
	{
		if (g_FloorGaugeValues[i] < maxValue - 0.1f)
		{
			allCleared = false;
			break;
		}
	}

	if (allCleared)
	{
		StartFade(SCENE_ANM_WIN);
	}

	// --- 敗北判定 ---
	if (g_Clock->Update() || g_ScareGauge->GetValue() <= 0.0f)
	{
		hal::dout << "敗北条件を満たしました" << std::endl;
		StartFade(SCENE_ANM_LOSE);
	}

	UI_ScareCombo_Update();
	g_FloorNumber->SetNumber(currentFloor + 1);

	// --- 階段ガイドの制御 ---
	bool onStairs = false;
	int targetFloor = 0;

	Ghost* ghost = GetGhost();

	if (ghost && !ghost->GetIsTransformed())
	{
		XMFLOAT3 pos = ghost->GetPos();
		FIELD_TYPE blockType = Field_GetBlockType(pos.x, pos.z);
		int floorIndex = Field_GetCurrentFloor();

		if (blockType == FIELD_STAIRS_UP)
		{
			if (floorIndex < MAP_FLOORS - 1)
			{
				onStairs = true;
				targetFloor = floorIndex + 2;
			}
		}
		else if (blockType == FIELD_STAIRS_DOWN)
		{
			if (floorIndex > 0)
			{
				onStairs = true;
				targetFloor = floorIndex;
			}
		}

		// 頭上ガイドの座標計算
		if (onStairs)
		{
			g_ShowGuideFloor = true;

			XMFLOAT3 headPos = pos;
			headPos.y += 2.0f;

			XMFLOAT2 screenPos = WorldToScreen(headPos);

			// ★修正: SetPos には XMFLOAT2 を渡す
			if (g_GuideFloorNum)
			{
				g_GuideFloorNum->SetPos(XMFLOAT2(screenPos.x - 25.0f, screenPos.y));
				g_GuideFloorNum->SetNumber(targetFloor);
			}

			if (g_GuideFloorF)
			{
				g_GuideFloorF->SetPos(XMFLOAT2(screenPos.x + 25.0f, screenPos.y));
			}
		}
		else
		{
			g_ShowGuideFloor = false;
		}
	}

	// クリックガイドの点滅
	if (onStairs)
	{
		static float flash = 0.0f;
		flash += 0.1f;
		float alpha = 0.5f + sinf(flash) * 0.5f;

		// ★修正: SetColor には XMFLOAT4 を渡す
		if (g_GuideClick)
			g_GuideClick->SetColor(XMFLOAT4(1.0f, 1.0f, 1.0f, alpha));
	}
	else
	{
		if (g_GuideClick)
			g_GuideClick->SetColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f));
	}
}

//----------------------------
//UI描画
//----------------------------
void UI_Draw(void)
{
	g_Clock->Draw();
	g_ScareGauge->Draw();
	UI_ScareCombo_Draw();

	if (g_FloorNumber) g_FloorNumber->Draw();
	if (g_FloorTextF) g_FloorTextF->Draw();

	// クリックガイド
	if (g_GuideClick) g_GuideClick->Draw();

	// 階層移動ガイド
	if (g_ShowGuideFloor)
	{
		if (g_GuideFloorNum) g_GuideFloorNum->Draw();
		if (g_GuideFloorF) g_GuideFloorF->Draw();
	}
}

//----------------------------
//UI終了
//----------------------------
void UI_Finalize(void)
{
	delete g_Clock;
	delete g_ScareGauge;
	delete g_Reticle;
	UI_ScareCombo_Finalize();

	if (g_FloorNumber) { delete g_FloorNumber; g_FloorNumber = nullptr; }
	if (g_FloorTextF) { delete g_FloorTextF; g_FloorTextF = nullptr; }

	if (g_GuideClick) { delete g_GuideClick; g_GuideClick = nullptr; }
	if (g_GuideFloorNum) { delete g_GuideFloorNum; g_GuideFloorNum = nullptr; }
	if (g_GuideFloorF) { delete g_GuideFloorF; g_GuideFloorF = nullptr; }
}

void AddScareGauge(float value)
{
	if (g_ScareGauge)
	{
		g_ScareGauge->AddValue(value);
	}
}