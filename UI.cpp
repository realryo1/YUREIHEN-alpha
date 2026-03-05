#pragma execution_character_set("utf-8")

#include "camera.h"
#include "UI.h"
#include "debug_ostream.h"
#include "keyboard.h"
#include "fade.h"
#include "UI_scarecombo.h"
#include "UI_messageframe.h"
#include "field.h"
#include "define.h"
//#include "sprite.h"
#include "ghost.h"
#include "font.h"
#include "furniture.h"
#include "result.h"
#include "WinAnim.h"
#include "game.h"
#include "Tutorial_Object.h"
#include "UI_RetryMenu.h"

// グローバル変数
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;
static DWORD g_LastScoreUpdateTime = 0;

static FontRenderer* g_PossessGuideFont = nullptr;
static std::string g_PossessGuideText = "";
static Sprite* g_PossessGuideBG = nullptr;

static Sprite* g_FloorNumberBG = nullptr;
static Number* g_FloorNumber = nullptr;

//!@ 仮で残り時間表示
static Number* g_RemainingTimeNum = nullptr;

// クリックガイド用
static Sprite* g_GuideClick = nullptr;

// 階層移動ガイド用
static Number* g_GuideFloorNum = nullptr;
static Sprite* g_GuideFloorF = nullptr;
static bool g_ShowGuideFloor = false;

// チュートリアル用
static bool g_IsTutorial = false;
static bool g_TutorialCompleted = false;
static int g_TutorialLastFloor = -1;
static Sprite* g_pTutorialBG = nullptr;
static FontRenderer* g_pTutorialFont = nullptr;

static MessageFrame* g_pMessageFrame = nullptr;

static std::string g_LastPossessGuideText = "";//文字列記憶

// 各階層のゲージ値を保存する配列
static float g_FloorGaugeValues[MAP_FLOORS];
// 前フレームの階層を記憶しておく変数
static int g_LastFrameFloor = 0;
// 全階の残り時	間の累計（リザルト用）
static float	 g_AccumulatedTime = 0.0f;


// 3D座標 -> 2Dスクリーン座標変換
static XMFLOAT2 WorldToScreen(const XMFLOAT3& worldPos)
{
	Camera* camera = GetCamera();
	// 明示的なコンストラクタを使用
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
// UI初期化
//----------------------------
void UI_Initialize(void)
{
	// 恐怖コンボを最初に初期化
	UI_ScareCombo_Initialize();

	g_Clock = new Timer(
		{ CLOCK_POS_X, CLOCK_POS_Y },
		{ CLOCK_SIZE, CLOCK_SIZE },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\clock.png",
		2, 1,
		CLOCK_MIN, CLOCK_MAX
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

	g_ScareGauge = new Gauge(
		{ SCREEN_WIDTH - 250.0f, 50.0f },
		{ GAUGE_SIZE, GAUGE_SIZE },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\gauge.png",
		3, 1,
		0.0f, SCARE_GAUGE_MAX,
		2, 0
	);

	// 現在の階層表示
	g_FloorNumberBG = new Sprite(
		{ CLOCK_POS_X, CLOCK_POS_Y + 160.0f },
		{ 200.0f, 200.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\kanban.png"
	);

	g_FloorNumber = new Number(
		{ CLOCK_POS_X, CLOCK_POS_Y + 200.0f },
		{ 60.0f, 60.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		50.0f
	);
	g_FloorNumber->SetNumber(1);
	g_FloorNumber->SetShowF(true);

	// ゲージ管理を初期化
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		// 階層ごとに初期値を設定
		if (i == 2) // 3階（インデックス2）
		{
			g_FloorGaugeValues[i] = 60.0f;
		}
		else // 1階、2階
		{
			g_FloorGaugeValues[i] = 50.0f;
		}
	}

	g_LastFrameFloor = Field_GetCurrentFloor();
	g_ScareGauge->SetValue(g_FloorGaugeValues[g_LastFrameFloor]);
	g_AccumulatedTime = 0.0f;

	g_PossessGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 100.0f },
		52.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		""
	);

	g_pMessageFrame = new MessageFrame(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 100.0f },
		L"asset\\texture\\massageframe.png"
	);
}

//----------------------------
// UI更新
//----------------------------
void UI_Update(void)
{
	// 階層が変わったかチェック
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor != g_LastFrameFloor)
	{
		// 前フレームの階層のゲージ値を保存
		if (g_LastFrameFloor >= 0 && g_LastFrameFloor < MAP_FLOORS)
		{
			g_FloorGaugeValues[g_LastFrameFloor] = g_ScareGauge->GetValue();
		}

		// 新しい階層のゲージ値を復元
		if (currentFloor >= 0 && currentFloor < MAP_FLOORS)
		{
			g_ScareGauge->SetValue(g_FloorGaugeValues[currentFloor]);
		}

		g_LastFrameFloor = currentFloor;
	}
	// --- 敗北条件 ---
#if STOP_TIMER_BUSTER
	bool timeEnded = false;
#else
	// 3階（index:2 / START_FLOOR-1）は時間制限なし
	bool timeEnded = false;
	if (currentFloor != (START_FLOOR - 1) && !Game_IsFloorExitAnimActive())
	{
		timeEnded = g_Clock->Update();
	}
#endif

	if ((timeEnded || g_ScareGauge->GetValue() <= 0.0f) && !UI_RetryMenu_IsActive() && !Game_IsLoseAnimActive())
	{
		Game_StartLoseAnim();
	}

	UI_ScareCombo_Update();

	if (g_FloorNumber)
	{
		g_FloorNumber->SetNumber(Field_GetCurrentFloor() + 1);
	}

	Ghost* ghost = GetGhost();

	// 家具憑依・アクションガイドの表示制御
	g_PossessGuideText = "";
	if (Game_IsFloorExitAnimActive())
	{
		if (Game_IsCamOverrideActive())
		{
			g_PossessGuideText = "バスターず「うわーっ！逃げろ～！」";
		}
		else
		{
			g_PossessGuideText = "下の階に逃げたバスターズを追いかけよう！";
		}
	}
	else if (ghost)
	{
		GHOST_STATE state = ghost->GetState();
		int furnitureIdx = ghost->GetInRangeNum();

		if (state == GS_CAUGHT)
		{
			g_PossessGuideText = "スペース連打で逃げろ！";
		}
		else if (state == GS_FURNITURE_FOUND)
		{
			Furniture* pFurniture = GetFurniture(furnitureIdx);
			if (pFurniture)
			{
				std::string name = GetBlockNameJa(pFurniture->GetBlockID());
				g_PossessGuideText = "スペース：" + name + "に憑依";
			}
			else
			{
				g_PossessGuideText = "スペース：家具に憑依";
			}
		}
		else if (state == GS_TRANSFORM || state == GS_SCARE)
		{
			Furniture* pFurniture = GetFurniture(furnitureIdx);
			if (pFurniture)
			{
				if (pFurniture->IsCoolingDown())
				{
					int sec = (int)ceilf(pFurniture->GetCooldownTimer());
					g_PossessGuideText = "再使用まで あと " + std::to_string(sec) + " 秒";
				}
				else
				{
					FURNITURE_ACTION action = pFurniture->GetActionType();
					switch (action)
					{
					case ACTION_SCARE: g_PossessGuideText = "[スペース] 驚かせ"; break;
					case ACTION_LURE:  g_PossessGuideText = "[スペース] 引き寄せ [WASD] ゆっくり移動"; break;
					case ACTION_STOP:  g_PossessGuideText = "[スペース] 気絶"; break;
					}
				}
			}
		}
		else if (state == GS_MOVING)
		{
			// クールダウン中の家具が近くにあるかチェック
			float minCooldownDist = FURNITURE_DETECTION_RANGE;
			int closestCooldownIdx = -1;

			for (int i = 0; i < FURNITURE_NUM; i++)
			{
				Furniture* pFurniture = GetFurniture(i);
				if (pFurniture && pFurniture->IsCoolingDown())
				{
					float dist = pFurniture->GetDistanceToGhost();
					if (dist <= minCooldownDist)
					{
						minCooldownDist = dist;
						closestCooldownIdx = i;
					}
				}
			}

			if (closestCooldownIdx != -1)
			{
				int sec = (int)ceilf(GetFurniture(closestCooldownIdx)->GetCooldownTimer());
				g_PossessGuideText = "再使用まで あと " + std::to_string(sec) + " 秒";
			}
		}
	}

	if (g_PossessGuideFont)
	{
		// 前回と違う文字列になったときだけ更新処理を行う
		if (g_PossessGuideText != g_LastPossessGuideText)
		{
			g_PossessGuideFont->SetText(g_PossessGuideText);
			g_LastPossessGuideText = g_PossessGuideText; // 記憶更新
		}
	}

	if (g_pMessageFrame)
	{
		if (!g_PossessGuideText.empty())
		{
			g_pMessageFrame->ShowFrame(g_PossessGuideText.size() / 2);
		}
		else
		{
			g_pMessageFrame->HideFrame();
		}
	}

	// --- 階段ガイドの制御 ---
	bool onStairs = false;
	int targetFloor = 0;

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

		// 移動ガイドの座標計算
		if (onStairs)
		{
			g_ShowGuideFloor = true;

			XMFLOAT3 headPos = pos;
			headPos.y += 2.0f;

			XMFLOAT2 screenPos = WorldToScreen(headPos);

			// SetPos には XMFLOAT2 を渡す
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

	if (Keyboard_IsKeyDownTrigger(KK_P))
	{
		if(Keyboard_IsKeyDownTrigger(KK_O))
		{		//3桁のスコアを適当にセット（Time=50, Combo=5 → Score=250）
			WinAnim_SetResultData(150.0f, 5);

			//Debug用
			StartFade(SCENE_ANM_WIN);
		}
	}
	//if (Keyboard_IsKeyDownTrigger(KK_L))
	//{
	//	//Debug用
	//	StartFade(SCENE_ANM_LOSE);
	//}
}

//----------------------------
// UI描画
//----------------------------
void UI_Draw(void)
{
	UI_ScareCombo_Draw();

	if (g_FloorNumberBG) g_FloorNumberBG->Draw();
	if (g_FloorNumber) g_FloorNumber->Draw();
	if (g_Clock) g_Clock->Draw();
	if (g_ScareGauge) g_ScareGauge->Draw();
	if (g_pMessageFrame) g_pMessageFrame->Draw();
	if (g_PossessGuideFont) g_PossessGuideFont->Draw();
}

//----------------------------
// UI終了
//----------------------------
void UI_Finalize(void)
{
	delete g_Clock;
	delete g_ScareGauge;
	UI_ScareCombo_Finalize();

	if (g_PossessGuideFont) { delete g_PossessGuideFont; g_PossessGuideFont = nullptr; }
	if (g_pMessageFrame) { delete g_pMessageFrame; g_pMessageFrame = nullptr; }
	if (g_FloorNumberBG) { delete g_FloorNumberBG; g_FloorNumberBG = nullptr; }
	if (g_FloorNumber) { delete g_FloorNumber; g_FloorNumber = nullptr; }
	if (g_GuideClick) { delete g_GuideClick; g_GuideClick = nullptr; }
	if (g_GuideFloorNum) { delete g_GuideFloorNum; g_GuideFloorNum = nullptr; }
}

void AddScareGauge(float value)
{
	if (g_ScareGauge)
	{
		g_ScareGauge->AddValue(value);
	}
}

// =================================================================
// ゲージ状態判定とリセット
// =================================================================
bool UI_IsScareGaugeMax(void)
{
	if (g_ScareGauge)
	{
		return (g_ScareGauge->GetValue() >= g_ScareGauge->GetMaxValue());
	}
	return false;
}

void UI_ResetScareGauge(void)
{
	if (g_ScareGauge)
	{
		g_ScareGauge->SetValue(g_FloorGaugeValues[g_LastFrameFloor]);
	}
}

float UI_GetScareGauge(void)
{
	if (g_ScareGauge)
	{
		return g_ScareGauge->GetValue();
	}
	return 0.0f;
}

void UI_DecreaseRemainingTime(float penaltySeconds)
{
	if (g_Clock)
	{
		// 現在の経過時間を取得して、ペナルティ分を加算
		float currentTime = g_Clock->GetTime();
		g_Clock->SetTime(currentTime + penaltySeconds);
	}
}

void UI_ResetTimer(void)
{
	if (g_Clock)
	{
		g_Clock->Reset();
	}
}

void UI_AccumulateFloorTime(void)
{
	if (g_Clock)
	{
		float remainingTime = CLOCK_MAX - g_Clock->GetTime();
		if (remainingTime < 0.0f) remainingTime = 0.0f;
		g_AccumulatedTime += remainingTime;
	}
}

float UI_GetAccumulatedTime(void)
{
	return g_AccumulatedTime;
}

void UI_RefreshTimerForPause(void)
{
	if (g_Clock)
	{
		g_Clock->RefreshLastUpdateTime();
	}
}