#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
using namespace DirectX;
#include "shader.h"
#include "debug_ostream.h"
#include "game.h"
#include "field.h"
#include "texture.h"
#include "keyboard.h"
#include "scene.h"
#include "camera.h"
#include "sprite.h"
#include "UI.h"
#include "UI_PauseMenu.h"
#include "UI_Tutorial.h"
#include "ghost.h"
#include "furniture.h"
#include "fade.h"
#include "busters.h"
#include "Tutorial_Object.h"
#include "debugdraw.h"
#include "sound.h"
#include "minimap.h"
#include "mouse.h"
#include "light.h"
#include "model.h"
#include "UI_ScareCombo.h"
#include "WinAnim.h"
#include "UI_ScareCombo.h"
#include "UI_RetryMenu.h"
#include "LoseAnim.h"
#include <windows.h>
#include <string>
#include <cmath>

static AmbientLight* g_pAmbientLight = nullptr;
static SoundData* g_pBGM = nullptr;

static int g_NextFloorID = -1;

// =================================================================
// 敗北アニメーションの状態管理
// =================================================================
enum LOSE_ANIM_STATE
{
	LOSE_NONE = 0,
	LOSE_FADEOUT,
	LOSE_PLAYING,
};
static LOSE_ANIM_STATE g_LoseAnimState = LOSE_NONE;

// =================================================================
// フロア降下アニメーションのステートマシン
// =================================================================
enum FLOOR_EXIT_ANIM_STATE
{
	FLOOR_EXIT_NONE = 0,         // 通常状態
	FLOOR_EXIT_FADEOUT,          // フェードアウト中（未使用）
	FLOOR_EXIT_OVERVIEW,         // 俯瞰カメラ + バスターズ走行中
	FLOOR_EXIT_CAM_LERP,         // 俯瞰→プレイヤー視点へカメラ補間
	FLOOR_EXIT_PLAYER_WALK,      // プレイヤー操作に戻し階段を待つ
	FLOOR_EXIT_FADEIN,           // フェードイン中（フロア移行フェード）
};

static FLOOR_EXIT_ANIM_STATE g_FloorExitState = FLOOR_EXIT_NONE;
static int  g_FloorExitTimer      = 0;
static bool g_FloorTransferDone   = false;
static bool g_FloorExitAnimRequested = false;
static bool g_NeedRestoreMouseMode = false; // 階層移行後にマウスをRELATIVEモードへ戻すフラグ
static int  g_OverviewFrameCount  = 0;
static XMFLOAT3 g_OverviewCameraPos = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_StairsPosForAnim  = { 0.0f, 0.0f, 0.0f };
static int      g_FloorBeforeExit   = -1;
static XMFLOAT3 g_LerpStartCamPos   = { 0.0f, 0.0f, 0.0f };
static XMFLOAT3 g_LerpStartAtPos    = { 0.0f, 0.0f, 0.0f };
static float    g_CamLerpT          = 0.0f;
static const float CAM_LERP_SPEED   = 0.012f;

// 現在位置から最も近い下り階段(ID5/6)座標を取得
static XMFLOAT3 GetNearestExitStairsPos(const XMFLOAT3& fromPos, int floor)
{
	std::vector<XMFLOAT3> stairs = Field_GetStairsExitPositions(floor);
	if (stairs.empty())
	{
		return Field_GetMarker97WorldPos(floor);
	}

	XMFLOAT3 nearest = stairs[0];
	float minDistSq = 99999999.0f;
	for (const auto& p : stairs)
	{
		float dx = p.x - fromPos.x;
		float dz = p.z - fromPos.z;
		float distSq = dx * dx + dz * dz;
		if (distSq < minDistSq)
		{
			minDistSq = distSq;
			nearest = p;
		}
	}
	return nearest;
}

static void SetFloorExitMarkerVisible(bool visible)
{
	TutorialMarker* marker = GetTutorialMarker();
	if (!marker) return;
	if (visible)
	{
		// 全階段座標をリストで渡す（全ブロックにBillboard表示）
		std::vector<XMFLOAT3> stairsList;
		if (g_FloorBeforeExit == 0)
		{
			stairsList = Field_GetFloor1ExitPositions(g_FloorBeforeExit);
		}
		else if (g_FloorBeforeExit > 0)
		{
			stairsList = Field_GetStairsExitPositions(g_FloorBeforeExit);
		}
		// フォールバック：リストが空なら g_StairsPosForAnim の単一座標を使う
		if (stairsList.empty())
			stairsList.push_back(g_StairsPosForAnim);

		marker->SetPositions(stairsList);
	}
	marker->SetVisible(visible);
}

// バスターズの真上に俯瞰カメラをセット
static void SetupOverviewCamera(void)
{
	XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
	Busters* buster = GetBusters();
	if (buster)
	{
		focusPos = buster->GetPos();
	}
	else
	{
		Ghost* ghost = GetGhost();
		if (ghost) focusPos = ghost->GetPos();
	}

	XMFLOAT3 camPos  = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
	XMFLOAT3 atPos   = { focusPos.x, focusPos.y,         focusPos.z + 0.01f };
	g_OverviewCameraPos = camPos;

	Camera* cam = GetCamera();
	if (cam) cam->UpdateView(camPos, atPos);
}


bool Game_IsFloorExitAnimActive(void)
{
	return g_FloorExitState != FLOOR_EXIT_NONE;
}

bool Game_IsCamOverrideActive(void)
{
	return g_FloorExitState == FLOOR_EXIT_OVERVIEW
	    || g_FloorExitState == FLOOR_EXIT_CAM_LERP;
}

bool* Game_GetEnbanTouchedPtr(void)
{
	return TutorialObject_GetEnbanTouchedPtr();
}



void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// フロア降下アニメ関連の状態を毎回初期化
	g_NextFloorID = -1;
	g_FloorExitState = FLOOR_EXIT_NONE;
	g_FloorExitTimer = 0;
	g_FloorTransferDone = false;
	g_FloorExitAnimRequested = false;
	g_NeedRestoreMouseMode = false;
	g_OverviewFrameCount = 0;
	g_OverviewCameraPos = { 0.0f, 0.0f, 0.0f };
	g_StairsPosForAnim = { 0.0f, 0.0f, 0.0f };
	g_FloorBeforeExit = -1;
	g_LerpStartCamPos = { 0.0f, 0.0f, 0.0f };
	g_LerpStartAtPos = { 0.0f, 0.0f, 0.0f };
	g_CamLerpT = 0.0f;

	// 敗北アニメ初期化
	g_LoseAnimState = LOSE_NONE;

	g_pAmbientLight = new AmbientLight(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

	Camera_Initialize();
	Ghost_Initialize(pDevice, pContext);
	Field_Initialize(pDevice, pContext);
	UI_Initialize();
	Furniture_Initialize();
	Busters_Initialize();
	Minimap_Initialize();
	DebugDraw_Initialize();

	TutorialObject_Initialize();
	SetFloorExitMarkerVisible(false);

	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM) PlaySound(g_pBGM, true);

	UI_PauseMenu_Initialize(pDevice, pContext);
	UI_Tutorial_Initialize(pDevice, pContext);
	UI_RetryMenu_Initialize(pDevice, pContext);

	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	Mouse_SetVisible(false);
}

void Game_Update(void)
{
	FADESTAT fadeState = GetFadeState();

	// -------------------------------------------------------
	// リトライメニュー表示中はゲーム更新を止める
	// -------------------------------------------------------
	if (UI_RetryMenu_IsActive())
	{
		UI_RetryMenu_Update();
		return;
	}

	// -------------------------------------------------------
	// 敗北アニメーション再生中
	// -------------------------------------------------------
	if (g_LoseAnimState != LOSE_NONE)
	{
		// -------------------------------------------------------
		// 敗北アニメーション（フェード遷移＋再生）
// -------------------------------------------------------
		if (g_LoseAnimState == LOSE_FADEOUT)
		{
			// フェードアウト完了を待つ
			if (fadeState == FADE_MAX)
			{
				// Loseアニメーションを初期化・再生開始
				Animation_Lose_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
				Fade_StartIn();
				g_LoseAnimState = LOSE_PLAYING;
			}
			return;
		}

		if (g_LoseAnimState == LOSE_PLAYING)
		{
			Animation_Lose_Update();
			return;
		}
	}

	// -------------------------------------------------------
	// フロア降下アニメーションのステートマシン
	// -------------------------------------------------------
	if (g_FloorExitState != FLOOR_EXIT_NONE)
	{
		fadeState = GetFadeState();

		UI_PauseMenu_Update();
		if (UI_PauseMenu_IsPaused())
		{
			return;
		}

		switch (g_FloorExitState)
		{
		case FLOOR_EXIT_OVERVIEW:
			{
				Busters_Update();

				if (g_OverviewFrameCount > 1)
				{
					Busters_DeleteFirstArrived();
				}

				XMFLOAT3 focusPos = { 0.0f, 0.0f, 0.0f };
				Busters* buster = GetBusters();
				if (buster) focusPos = buster->GetPos();
				else { Ghost* ghost = GetGhost(); if (ghost) focusPos = ghost->GetPos(); }
				XMFLOAT3 camPos = { focusPos.x, focusPos.y + 20.0f, focusPos.z };
				XMFLOAT3 atPos  = { focusPos.x, focusPos.y,         focusPos.z + 0.01f };
				Camera* cam = GetCamera();
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);
				UI_Update();
				g_OverviewFrameCount++;
				if (g_OverviewFrameCount > 1 && GetBusters() == nullptr)
				{
					// 変身中の場合は強制解除する
					Ghost_ForceExitTransform();

					g_LerpStartAtPos = atPos;

					{
						Ghost* ghostForAt = GetGhost();
						if (ghostForAt)
						{
							XMFLOAT3 gp;
							if (ghostForAt->m_PreTransformPos.x != 0.0f || ghostForAt->m_PreTransformPos.z != 0.0f)
							{
								gp = ghostForAt->m_PreTransformPos;
								gp.y = GHOST_POS_Y;
							}
							else
							{
								gp = ghostForAt->GetPos();
							}
							Camera_SetTargetPos(gp);
						}
					}
					g_LerpStartCamPos = camPos;
					g_CamLerpT        = 0.0f;
					g_OverviewFrameCount = 0;
					g_FloorExitState  = FLOOR_EXIT_CAM_LERP;
				}
			}
			return;

		case FLOOR_EXIT_CAM_LERP:
			{
				SetFloorExitMarkerVisible(false);
				g_CamLerpT += CAM_LERP_SPEED;
				if (g_CamLerpT > 1.0f) g_CamLerpT = 1.0f;

				Ghost* ghostLerp = GetGhost();
				if (ghostLerp && ghostLerp->m_InvincibleTimer > 0) ghostLerp->m_InvincibleTimer--;

				XMFLOAT3 dstCamPos = { 0.0f, 0.0f, 0.0f };
				XMFLOAT3 dstAtPos  = { 0.0f, 0.0f, 0.0f };
				if (ghostLerp)
				{
					XMFLOAT3 gp = ghostLerp->m_PreTransformPos;
					gp.y = GHOST_POS_Y;
					XMFLOAT3 targetPos = { gp.x, gp.y + CAMERA_OFFSET_Y, gp.z };
					dstAtPos = targetPos;

					Camera* cam = GetCamera();
					float pitchRad = XMConvertToRadians(cam ? cam->GetPitch() : 0.0f);
					float yawRad   = XMConvertToRadians(cam ? cam->GetYaw()   : 0.0f);
					float camX = -sinf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;
					float camY = -sinf(pitchRad) * CAMERA_DISTANCE + 0.1f;
					float camZ = -cosf(yawRad) * cosf(pitchRad) * CAMERA_DISTANCE;
					dstCamPos = { targetPos.x + camX, targetPos.y + camY, targetPos.z + camZ };
				}

				Camera* cam = GetCamera();
				XMFLOAT3 camPos =
				{
					g_LerpStartCamPos.x + (dstCamPos.x - g_LerpStartCamPos.x) * g_CamLerpT,
					g_LerpStartCamPos.y + (dstCamPos.y - g_LerpStartCamPos.y) * g_CamLerpT,
					g_LerpStartCamPos.z + (dstCamPos.z - g_LerpStartCamPos.z) * g_CamLerpT
				};
				XMFLOAT3 atPos =
				{
					g_LerpStartAtPos.x + (dstAtPos.x - g_LerpStartAtPos.x) * g_CamLerpT,
					g_LerpStartAtPos.y + (dstAtPos.y - g_LerpStartAtPos.y) * g_CamLerpT,
					g_LerpStartAtPos.z + (dstAtPos.z - g_LerpStartAtPos.z) * g_CamLerpT
				};
				if (cam) cam->UpdateView(camPos, atPos);
				Shader_SetCameraPos(camPos);

				float ex = camPos.x - dstCamPos.x;
				float ey = camPos.y - dstCamPos.y;
				float ez = camPos.z - dstCamPos.z;
				float distSq = ex * ex + ey * ey + ez * ez;
				if (g_CamLerpT >= 1.0f || distSq < 2.0f * 2.0f)
				{
					if (ghostLerp)
					{
						XMFLOAT3 syncPos = ghostLerp->m_PreTransformPos;
						syncPos.y = GHOST_POS_Y;
						ghostLerp->SetPos(syncPos);
						Camera_SetTargetPos(syncPos);
					}
					if (g_FloorBeforeExit == 0)
					{
						StartFade(SCENE_NONE);
						g_FloorExitState = FLOOR_EXIT_FADEIN;
					}
					else
					{
						SetFloorExitMarkerVisible(true);
						g_FloorExitState = FLOOR_EXIT_PLAYER_WALK;
					}
				}
				UI_Update();
			}
			return;

		case FLOOR_EXIT_PLAYER_WALK:
			{
				Camera_Update();
				Shader_SetCameraPos(GetCamera()->GetPos());
				Field_Update();
				UI_Update();
				TutorialObject_Update();
				Ghost_Update();

				Ghost* ghost = GetGhost();
				if (ghost && fadeState == FADE_NONE)
				{
					XMFLOAT3 gp = ghost->GetPos();
					int blockID = Field_GetRawBlockID(gp.x, gp.z);
					if (blockID == 5 || blockID == 6)
					{
						SetFloorExitMarkerVisible(false);
						StartFade(SCENE_NONE);
						g_FloorExitState = FLOOR_EXIT_FADEIN;
					}
				}
			}
			return;

		case FLOOR_EXIT_FADEIN:
			if (fadeState == FADE_MAX)
			{
				SetFloorExitMarkerVisible(false);
				if (g_FloorBeforeExit == END_FLOOR - 1 || g_FloorBeforeExit == 0)
				{
					UI_AccumulateFloorTime();
					{
						extern float UI_GetAccumulatedTime(void);
						extern int UI_ScareCombo_GetNumber(void);
						WinAnim_SetResultData(UI_GetAccumulatedTime(), UI_ScareCombo_GetNumber());
					}

					g_FloorExitState     = FLOOR_EXIT_NONE;
					g_FloorExitTimer     = 0;
					g_OverviewFrameCount = 0;
					g_FloorTransferDone  = false;
					g_FloorExitAnimRequested = false;
					g_FloorBeforeExit    = -1;
					SetScene(SCENE_ANM_WIN);
					Fade_StartIn();
				}
				else
				{
					int nextFloor = g_FloorBeforeExit - 1;
					if (g_FloorBeforeExit != (START_FLOOR - 1))
					{
						UI_AccumulateFloorTime();
					}
					Ghost* ghost = GetGhost();
					if (ghost)
					{
						Field_ChangeFloor(nextFloor);
						XMFLOAT3 spawnPos = GetGhostStartPos(nextFloor);
						ghost->ResetPos();
						ghost->SetPos(spawnPos);
						Camera_SetTargetPos(spawnPos);
					}
					Busters_SpawnOnFloor(nextFloor);
					UI_ResetScareGauge();
					UI_ResetTimer();
					AddScareGauge(BUSTERS_DEFOURT_GAUGE);
					Fade_StartIn();
					g_NeedRestoreMouseMode = true;
					g_FloorExitState     = FLOOR_EXIT_NONE;
					g_FloorExitTimer     = 0;
					g_FloorTransferDone  = false;
					g_FloorExitAnimRequested = false;
					g_FloorBeforeExit    = -1;
				}
			}
			return;

		default:
			break;
		}
		return;
	}

	// -------------------------------------------------------
	// 通常フェード処理
	// -------------------------------------------------------
	if (fadeState == FADE_MAX)
	{
		if (g_NextFloorID != -1) {
			LoadMapData(g_NextFloorID);
			GetGhost()->SetPos(XMFLOAT3(0, 0, 0));
			g_NextFloorID = -1;
			Fade_StartIn();
			g_NeedRestoreMouseMode = true;
		}
		return;
	}
	if (fadeState != FADE_NONE) return;

	// 階層移行後、フェードイン完了の最初のフレームでマウスを回復する
	if (g_NeedRestoreMouseMode)
	{
		g_NeedRestoreMouseMode = false;
		Mouse_ReacquireFocus();
		// マウス復帰直後に残存する累積値による異常回転を防ぐため数フレーム入力を読み飛ばす
		Camera* cam = GetCamera();
		if (cam) cam->SkipNextInput(5);
	}

	UI_PauseMenu_Update();
	if (UI_PauseMenu_IsPaused())
	{
		return;
	}

	if (Field_GetCurrentFloor() == 2)
	{
		UI_Tutorial_Update();
		TutorialObject_Update();


		if (UI_Tutorial_IsWaiting())
		{
			Camera_Update();
			Shader_SetCameraPos(GetCamera()->GetPos());
			Field_Update();
			Ghost_Update();
			Furniture_Update();
			UI_ScareCombo_Update();
			return;
		}

		if (UI_Tutorial_IsActive())
		{
			return;
		}
	}

	Camera_Update();
	Shader_SetCameraPos(GetCamera()->GetPos());
	Field_Update();
	UI_Update();
	Furniture_Update();
	Ghost_Update();
#if !STOP_TIMER_BUSTER
	// 3階のチュートリアル中（ページ表示中・テストプレイ待機中）は通常バスターズを更新しない
	if (!(Field_GetCurrentFloor() == 2 && (UI_Tutorial_IsActive() || UI_Tutorial_IsWaiting())))

	{
		Busters_Update();
	}
#endif

	// ゲージMAX判定：MAXになったらバスターズを階段へ走らせる
	// 驚かしアクション中(GS_SCARE)はアニメーション完了を待ってから判定する
	{
		Ghost* ghostForGauge = GetGhost();
		bool isScarePlaying = ghostForGauge &&
			(ghostForGauge->GetState() == GS_SCARE || ghostForGauge->GetState() == GS_TRANSFORM);
		if (g_FloorExitState == FLOOR_EXIT_NONE && GetFadeState() == FADE_NONE && !isScarePlaying)
		{
			if (UI_IsScareGaugeMax())
			{
				Ghost* ghostForExit = GetGhost();
				XMFLOAT3 basePos = ghostForExit ? ghostForExit->GetPos() : XMFLOAT3(0.0f, 0.0f, 0.0f);
				g_StairsPosForAnim = GetNearestExitStairsPos(basePos, Field_GetCurrentFloor());
				SetFloorExitMarkerVisible(false);
				SetupOverviewCamera();
				g_FloorExitState = FLOOR_EXIT_OVERVIEW;
				g_FloorExitTimer = 0;
				g_OverviewFrameCount = 0;
				g_FloorBeforeExit = Field_GetCurrentFloor();
				Busters_StartFloorExitAnim();
			}
		}
	}

	DebugDraw_Update();
}

void Game_Draw(void)
{
	SetDepthTest(true);

	Shader_SetAmbientLight(g_pAmbientLight);
	Shader_ClearPointLights();
	Ghost_SetLight();
	Busters_SetLight();
	Furniture_SetLight();
	TutorialBusters_SetLight();
	UI_Tutorial_SetLight();

	Field_Draw();
	// 3階のチュートリアル中（ページ表示中・テストプレイ待機中）は通常バスターズを描画しない
	if (!(Field_GetCurrentFloor() == 2 && (UI_Tutorial_IsActive() || UI_Tutorial_IsWaiting())))
	{
		Busters_Draw();
	}

	Furniture_Draw();
	Ghost_Draw();
	DebugDraw_Draw();

	// TutorialObject_Draw はマーカー（矢印）を全フロアで描画するため常時呼ぶ
	// 内部で3階専用オブジェクト（バスターズ・円盤）は Field_GetCurrentFloor() == 2 のときのみ描画される
	TutorialObject_Draw();

	SetDepthTest(false);

	Sprite_BeginDraw2D();
	UI_Draw();
	Minimap_Draw();
	TutorialObject_Draw2D();

	if (Field_GetCurrentFloor() == 2)
	{
		UI_Tutorial_Draw();
	}
	UI_PauseMenu_Draw();

	// 敗北アニメーション再生中は全画面にLoseアニメを描画
	if (g_LoseAnimState == LOSE_PLAYING)
	{
		Animation_Lose_Draw();
	}

	UI_RetryMenu_Draw();

	Sprite_EndDraw2D();
}

void Game_Finalize(void)
{
	SetFloorExitMarkerVisible(false);

	// 敗北アニメが再生中なら解放
	if (g_LoseAnimState == LOSE_PLAYING)
	{
		Animation_Lose_Finalize();
		g_LoseAnimState = LOSE_NONE;
	}

	if (g_pBGM) {
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}

	if (g_pAmbientLight) {
		delete g_pAmbientLight;
		g_pAmbientLight = nullptr;
	}

	UI_PauseMenu_Finalize();
	UI_Tutorial_Finalize();
	UI_RetryMenu_Finalize();

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Minimap_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();

	TutorialObject_Finalize();
}

// =================================================================
// 敗北アニメーション関連
// =================================================================
bool Game_IsLoseAnimActive(void)
{
	return g_LoseAnimState != LOSE_NONE;
}

void Game_StartLoseAnim(void)
{
	if (g_LoseAnimState != LOSE_NONE) return;

	// ゲームBGMを停止
	if (g_pBGM) StopSound(g_pBGM);

	// フェードアウト開始（SCENE_NONEでシーン遷移なし）
	StartFade(SCENE_NONE);
	g_LoseAnimState = LOSE_FADEOUT;
}

void Game_EndLoseAnim(void)
{
	if (g_LoseAnimState == LOSE_PLAYING)
	{
		Animation_Lose_Finalize();
	}
	g_LoseAnimState = LOSE_NONE;

	// ゲームBGMを再開
	if (g_pBGM) PlaySound(g_pBGM, true);
}

// =================================================================
// チュートリアル等でのフロア強制スキップ用
// =================================================================
void Game_ForceSkipFloor(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor <= 0) return;

	g_FloorBeforeExit = currentFloor;
	StartFade(SCENE_NONE);
	g_FloorExitState = FLOOR_EXIT_FADEIN;
}
