#include "ghost.h"
using namespace DirectX;
#include "sprite.h"
#include "sprite3d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "field.h"
#include "mouse.h"
#include "debug_ostream.h"
#include "camera.h"
#include "furniture.h"
#include "busters.h"
#include "Tutorial_Object.h"
#include "game.h"
#include "scene.h"
#include "UI.h"
#include "UI_scarecombo.h"
#include "define.h"
#include "sound.h"
#include <algorithm>
#include "shader.h"

Ghost* g_Ghost = NULL;
SoundData* g_pScareSound = nullptr;

// 現在の驚かし範囲（半径）を計算する関数
static float GetCurrentScareRange()
{
	int combo = UI_ScareCombo_GetNumber();
	if (combo < 1)
	{
		combo = 1;
	}

	return SCARE_COMBO_BASE_RADIUS + (combo - 1) * SCARE_COMBO_RADIUS_STEP;
}

// 現在のフロアにバスターズ（通常 or チュートリアル）が1体でもいるか
static bool HasAnyBusterOnCurrentFloor()
{
	if (Busters_GetCurrentFloorCount() > 0)
	{
		return true;
	}
	TutorialBusters* pTutBuster = GetTutorialBusters();
	if (pTutBuster && pTutBuster->GetState() != TB_STUN)
	{
		return true;
	}
	return false;
}

static float GetActionEffectiveRange(Furniture* pFurniture, float baseRange)
{
    if (!pFurniture)
    {
        return baseRange;
    }

    switch (pFurniture->GetActionType())
    {
    case ACTION_LURE:
        return baseRange * 2.0f;
    case ACTION_STOP:
        return BUSTERS_STOP_RANGE;
    default:
        return baseRange;
    }
}

// 変身解除後、壁や家具に埋まらない安全な位置を返すヘルパー関数
// 元の座標から4方向(+Z,-Z,+X,-X)へ1マス(1.0f)ずつオフセットして
// 壁・床なしでないグリッドを見つける。すべて失敗した場合は元の座標を返す。
static XMFLOAT3 CalcSafeExitPos(XMFLOAT3 origin)
{
	const float STEP = 1.0f;
	const float offsets[4][2] = { {0.0f, STEP}, {0.0f, -STEP}, {STEP, 0.0f}, {-STEP, 0.0f} };
	const float r = 0.4f;
	for (int i = 0; i < 4; ++i)
	{
		float cx = origin.x + offsets[i][0];
		float cz = origin.z + offsets[i][1];
		// 4隅すべてが壁でも床なしでもなければ安全
		if (!Field_IsWall(cx + r, origin.y, cz + r) && !Field_IsWall(cx + r, origin.y, cz - r) &&
			!Field_IsWall(cx - r, origin.y, cz + r) && !Field_IsWall(cx - r, origin.y, cz - r) &&
			!Field_IsOuterWall(cx + r, cz + r) && !Field_IsOuterWall(cx + r, cz - r) &&
			!Field_IsOuterWall(cx - r, cz + r) && !Field_IsOuterWall(cx - r, cz - r) &&
			!Field_IsNoFloor(cx + r, cz + r) && !Field_IsNoFloor(cx + r, cz - r) &&
			!Field_IsNoFloor(cx - r, cz + r) && !Field_IsNoFloor(cx - r, cz - r))
		{
			return { cx, origin.y, cz };
		}
	}
	return origin;
}

// 全家具のゴーストターゲットフラグをリセットする
static void ResetAllFurnitureGhostTarget()
{
	int count = GetFurnitureCount();
	for (int i = 0; i < count; i++)
	{
		Furniture* pF = GetFurniture(i);
		if (pF) pF->SetIsGhostTarget(false);
	}
}

// 円のサイズと位置を更新するヘルパー関数
static void UpdateRangeCircleState()
{
	if (!g_Ghost || !g_Ghost->m_pRangeCircle) return;

	Furniture* pFurniture = GetFurniture(g_Ghost->GetInRangeNum());
	float currentRange = GetCurrentScareRange();
	float actionRange = GetActionEffectiveRange(pFurniture, currentRange);

	// 円モデルの直径 = 半径 * 2
	g_Ghost->m_pRangeCircle->SetSize({ actionRange * 2.0f, 0.1f, actionRange * 2.0f });

	// 位置を家具に合わせる
	if (pFurniture)
	{
		XMFLOAT3 circlePos = pFurniture->GetPos();
		float groundY = pFurniture->GetGroundLevel();
		circlePos.y = groundY + 0.01f;
		g_Ghost->m_pRangeCircle->SetPos(circlePos);
	}
}

static bool CanTriggerScareNow(void)
{
	if (!TutorialObject_IsScareRequireBusterInRange())
	{
		return true;
	}

	if (!g_Ghost)
	{
		return false;
	}

	Furniture* pFurniture = GetFurniture(g_Ghost->GetInRangeNum());
	float currentRange = GetCurrentScareRange();
	float actionRange = GetActionEffectiveRange(pFurniture, currentRange);
	XMFLOAT3 ghostPos = g_Ghost->GetPos();

	if (Busters_IsAnyInRange(ghostPos, actionRange))
	{
		return true;
	}

	TutorialBusters* pTutBuster = GetTutorialBusters();
	if (pTutBuster && pTutBuster->GetState() != TB_STUN)
	{
		XMFLOAT3 tutPos = pTutBuster->GetPos();
		float dx = tutPos.x - ghostPos.x;
		float dz = tutPos.z - ghostPos.z;
		if (dx * dx + dz * dz <= actionRange * actionRange)
		{
			return true;
		}
	}

	return false;
}

static void UpdateLureFurnitureMovement()
{
    if (!g_Ghost)
    {
        return;
    }

    int furnitureIndex = g_Ghost->GetInRangeNum();
    if (furnitureIndex < 0)
    {
        return;
    }

    Furniture* pFurniture = GetFurniture(furnitureIndex);
    if (!pFurniture || pFurniture->GetActionType() != ACTION_LURE)
    {
        return;
    }

    float cameraYaw = Camera_GetYaw();
    float yawRad = XMConvertToRadians(cameraYaw);
    float forwardX = sinf(yawRad);
    float forwardZ = cosf(yawRad);
    float rightX = cosf(yawRad);
    float rightZ = -sinf(yawRad);

    float dirX = 0.0f;
    float dirZ = 0.0f;

    if (Keyboard_IsKeyDown(KK_W))
    {
        dirX += forwardX;
        dirZ += forwardZ;
    }
    if (Keyboard_IsKeyDown(KK_S))
    {
        dirX -= forwardX;
        dirZ -= forwardZ;
    }
    if (Keyboard_IsKeyDown(KK_D))
    {
        dirX += rightX;
        dirZ += rightZ;
    }
    if (Keyboard_IsKeyDown(KK_A))
    {
        dirX -= rightX;
        dirZ -= rightZ;
    }

    if (dirX == 0.0f && dirZ == 0.0f)
    {
        return;
    }

    float length = sqrtf(dirX * dirX + dirZ * dirZ);
    if (length <= 0.0f)
    {
        return;
    }

    dirX /= length;
    dirZ /= length;

    float moveSpeed = GHOST_MAX_SPEED * LURE_POSSESSED_SPEED_RATIO;
    XMFLOAT3 currentPos = pFurniture->GetPos();
    XMFLOAT3 newPos = currentPos;

    float r = 0.4f;

    float nextX = newPos.x + dirX * moveSpeed;
    bool hitX = Field_IsOuterWall(nextX + r, newPos.z + r) ||
        Field_IsOuterWall(nextX + r, newPos.z - r) ||
        Field_IsOuterWall(nextX - r, newPos.z + r) ||
        Field_IsOuterWall(nextX - r, newPos.z - r);

    if (!hitX)
    {
        newPos.x = nextX;
    }

    float nextZ = newPos.z + dirZ * moveSpeed;
    bool hitZ = Field_IsOuterWall(newPos.x + r, nextZ + r) ||
        Field_IsOuterWall(newPos.x + r, nextZ - r) ||
        Field_IsOuterWall(newPos.x - r, nextZ + r) ||
        Field_IsOuterWall(newPos.x - r, nextZ - r);

    if (!hitZ)
    {
        newPos.z = nextZ;
    }

    if (newPos.x != currentPos.x || newPos.z != currentPos.z)
    {
        pFurniture->SetPos(newPos);
        pFurniture->SetBasePos(newPos);
    }
}

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_Ghost = new Ghost(
		GetGhostStartPos(),
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 180.0f, 0.0f },
		"asset\\model\\ghost.fbx"
	);

	// 検出範囲を表示する円を初期化
	if (g_Ghost)
	{
		g_Ghost->m_pLight = new PointLight(
			TRUE,
			XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f),		// 位置 (Point Light)
			XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f),		// 方向 (Dummy)
			XMFLOAT4(1.0f, 1.0f, 0.9f, 1.0f),		// 拡散光
			6.0f,									// 減衰距離 (Range)
			0.8f									// 強度 (Intensity)
		);

		XMFLOAT3 circlePos = g_Ghost->GetPos();
		circlePos.y = 0.01f;

		// 初期サイズ
		float initRange = 2.0f;

		g_Ghost->m_pRangeCircle = new Sprite3D(
			circlePos,
			{ initRange * 2.0f, 0.1f, initRange * 2.0f },
			{ 0.0f, 0.0f, 0.0f },
			"asset\\model\\circle.fbx"
		);

		if (g_Ghost->m_pRangeCircle)
		{
			g_Ghost->m_pRangeCircle->SetColor(0.0f, 1.0f, 0.0f, 0.5f);
		}
	}

	Camera_SetTargetPos(g_Ghost->GetPos());

}

void Ghost_Update(void)
{
	if (!g_Ghost) return;

	if (g_Ghost->m_InvincibleTimer > 0)
	{
		g_Ghost->m_InvincibleTimer--;
	}

	if (g_Ghost->m_ScareCooldown > 0)
	{
		g_Ghost->m_ScareCooldown--;
	}

	// 「前フレームで照らされていて、今フレームは照らされていない」＝ 脱出成功
	//if (g_Ghost->m_PrevIsIlluminated && !g_Ghost->m_IsIlluminated)
	//{
	//	g_Ghost->m_InvincibleTimer = 300; // 5秒間無敵 (60FPS × 5秒)
	//}

	// 状態を保存し、今のフラグをリセット（バスターズ側で毎フレーム判定してtrueにするため）
	g_Ghost->m_PrevIsIlluminated = g_Ghost->m_IsIlluminated;
	g_Ghost->m_IsIlluminated = false;

	switch (g_Ghost->GetState())
	{
	case GS_MOVING:
		g_Ghost->SetIsDraw(true);
		g_Ghost->Move();
		g_Ghost->FurnitureSearch();
		g_Ghost->FloorMove();
		break;

	case GS_FURNITURE_FOUND:
		g_Ghost->SetIsDraw(true);
		g_Ghost->Move();
		g_Ghost->FurnitureSearch();
		g_Ghost->FloorMove();

		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			if (!HasAnyBusterOnCurrentFloor() || Game_IsFloorExitAnimActive())
			{
				break;
			}

			UpdateRangeCircleState();

			// 変身前の実際のゴーストの位置を記録する（カメラ復帰に使用）
			g_Ghost->m_PreTransformPos = g_Ghost->GetPos();

			g_Ghost->SetState(GS_TRANSFORM);
			g_Ghost->SetIsTransformed(true);
			g_Ghost->SetVelocity({ 0.0f, 0.0f, 0.0f });
			g_Ghost->m_HasIncreasedMultiplier = false;
			g_Ghost->SetIsDraw(false);
		}
		break;

	case GS_TRANSFORM:
		if (!HasAnyBusterOnCurrentFloor() || Game_IsFloorExitAnimActive())
		{
			ResetAllFurnitureGhostTarget();
			UpdateRangeCircleState();
			XMFLOAT3 autoExitPos = CalcSafeExitPos(g_Ghost->GetPos());
			g_Ghost->ResetPos();
			g_Ghost->SetPos(autoExitPos);
			g_Ghost->SetState(GS_MOVING);
			g_Ghost->SetIsDraw(true);
			break;
		}

		UpdateLureFurnitureMovement();
		g_Ghost->Transforming();

		if (g_Ghost->m_pRangeCircle)
		{
			Furniture* pFurniture = GetFurniture(g_Ghost->GetInRangeNum());
			if (pFurniture)
			{
				XMFLOAT3 circlePos = pFurniture->GetPos();
				float groundY = pFurniture->GetGroundLevel();
				circlePos.y = groundY + 0.01f;
				g_Ghost->m_pRangeCircle->SetPos(circlePos);
			}
		}

		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			if (TutorialObject_IsScareEnabled() && CanTriggerScareNow() && g_Ghost->m_ScareCooldown <= 0)
			{
				g_Ghost->SetState(GS_SCARE);
				g_Ghost->ScareStart();
			}
		}

		if (Keyboard_IsKeyDownTrigger(KK_E))
		{

			UpdateRangeCircleState();

			// 変身解除後、壁や家具に埋まらない安全な隣接マスへ移動させる
			XMFLOAT3 exitPos = CalcSafeExitPos(g_Ghost->GetPos());

			g_Ghost->ResetPos();
			g_Ghost->SetPos(exitPos);
			g_Ghost->SetState(GS_MOVING);
			g_Ghost->SetIsDraw(true);
		}
		break;

	case GS_SCARE:
		if (!HasAnyBusterOnCurrentFloor() || Game_IsFloorExitAnimActive())
		{
			ResetAllFurnitureGhostTarget();
			UpdateRangeCircleState();
			XMFLOAT3 scareAutoExit = CalcSafeExitPos(g_Ghost->GetPos());
			g_Ghost->ResetPos();
			g_Ghost->SetPos(scareAutoExit);
			g_Ghost->SetState(GS_MOVING);
			g_Ghost->SetIsDraw(true);
			break;
		}

		g_Ghost->Transforming();

		if (FurnitureScareEnded(g_Ghost->GetInRangeNum()))
		{
			UpdateRangeCircleState();

			// 変身解除後、壁や家具に埋まらない安全な隣接マスへ移動させる
			XMFLOAT3 scareExitPos = CalcSafeExitPos(g_Ghost->GetPos());

			g_Ghost->ResetPos();
			g_Ghost->SetPos(scareExitPos);
			g_Ghost->SetState(GS_MOVING);
			g_Ghost->m_ScareCooldown = 60; // 驚かし後1秒のクールタイム
			g_Ghost->SetIsDraw(true);
		}
		break;

	case GS_CAUGHT:
		g_Ghost->SetIsDraw(true);

		// 移動も家具検知もさせない（完全に行動不能）

		// --- 毎秒のペナルティ処理 ---
		g_Ghost->m_CaughtPenaltyTimer++;
		if (g_Ghost->m_CaughtPenaltyTimer >= 60) // 1秒(60フレーム)経過
		{
			g_Ghost->m_CaughtPenaltyTimer = 0;

			AddScareGauge(-5.0f);
		}

		// --- 連打脱出の処理 ---
		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			g_Ghost->m_EscapeTapCount++;

			// 10回連打したら脱出成功！
			if (g_Ghost->m_EscapeTapCount >= 10)
			{
				g_Ghost->m_EscapeTapCount = 0;
				g_Ghost->m_CaughtPenaltyTimer = 0;

				// 脱出ボーナス: 5秒間無敵にする
				g_Ghost->SetInvincible(300);

				// ペナルティ: 残り時間 -10秒
				UI_DecreaseRemainingTime(10.0f);

				// 移動状態に戻る
				g_Ghost->SetState(GS_MOVING);
			}
		}

		break;

	default:
		break;
	}

	// P キーでデバッグ出力
	if (Keyboard_IsKeyDownTrigger(KK_P))
	{
		if (g_Ghost)
		{
			XMFLOAT3 pos = g_Ghost->GetPos();
		}

		// 現在のSCENEを名前付きで出力
		const char* sceneNames[] = {
			"SCENE_TITLE", "SCENE_GAME", "SCENE_RESULT",
			"SCENE_ANM_LOGO", "SCENE_ANM_OP", "SCENE_ANM_WIN",
			"SCENE_ANM_LOSE", "SCENE_ANM_LOSE_ED"
		};
		SCENE currentScene = GetScene();
		const char* sceneName = (currentScene >= 0 && currentScene < SCENE_MAX)
			? sceneNames[currentScene] : "UNKNOWN";
	}

	if (g_Ghost)
	{
		// 俯瞰・補間中のみカメラ追従を止める
		// PLAYER_WALK以降は通常追従に戻す
		if (!Game_IsCamOverrideActive())
		{
			Camera_SetTargetPos(g_Ghost->GetPos());
		}
	}
}

void Ghost_Draw(void)
{
	if (g_Ghost)
	{
		g_Ghost->Draw();
	}

	if (g_Ghost && g_Ghost->m_pRangeCircle &&
		(g_Ghost->GetState() == GS_TRANSFORM || g_Ghost->GetState() == GS_SCARE))
	{
		g_Ghost->m_pRangeCircle->Draw();
	}
}

void Ghost_Finalize(void)
{
	if (g_pScareSound)
	{
		UnloadSound(g_pScareSound);
		g_pScareSound = nullptr;
	}

	if (g_Ghost)
	{
		delete g_Ghost;
		g_Ghost = NULL;
	}
}

void Ghost_SetLight(void)
{
	if (!g_Ghost || !g_Ghost->m_pLight) return;

	// 毎フレーム Ghost の最新位置をライトに反映
	XMFLOAT3 ghostPos = g_Ghost->GetPos();
	g_Ghost->m_pLight->SetPosition(ghostPos.x, ghostPos.y, ghostPos.z);

	// シェーダーにライト情報を設定
	Shader_AddPointLight(g_Ghost->m_pLight);
}

// ========== Ghost クラスメソッドの実装 ==========

void Ghost::Transforming(void)
{
	Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
	if (pFurniture)
	{
		// XZ座標のみ家具に合わせる（Y座標は変更しない）
		XMFLOAT3 furniturePos = pFurniture->GetPos();
		SetPosX(furniturePos.x);
		SetPosZ(furniturePos.z);
	}

	Busters* pBuster = GetBusters();
	if (pBuster)
	{
		XMFLOAT3 busterPos = pBuster->GetPos();
		XMFLOAT3 ghostPos = GetPos();
		XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
		XMVECTOR busterVec = XMLoadFloat3(&busterPos);
		XMVECTOR distVec = XMVectorSubtract(busterVec, ghostVec);
		float distance = XMVectorGetX(XMVector3Length(distVec));

		float currentRange = GetCurrentScareRange();

		if (distance <= currentRange)
		{
			pBuster->SetIsGhostDiscover(true);
		}
		else
		{
			pBuster->SetIsGhostDiscover(false);
		}
	}
}

void Ghost::ScareStart(void)
{
	FurnitureScareStart(m_InRangeFurnitureNum);

	Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
	if (!pFurniture) return;

	// 家具IDごとの耐性倍率を取得し、使用回数をインクリメント
	int furnitureBlockID = pFurniture->GetBlockID();
	float resistanceMult = Furniture_GetResistanceMultiplier(furnitureBlockID);
	Furniture_IncrementUseCount(furnitureBlockID);


	if (g_pScareSound)
	{
		PlaySound(g_pScareSound, false);
	}

	XMFLOAT3 ghostPos = GetPos();

	FURNITURE_ACTION action = pFurniture->GetActionType();
	float currentRange = GetCurrentScareRange();

	float backScareAngle = 140.0f;      // 背後と判定する角度（180度にすると真横も背後扱いになります。少し狭めの140度を推奨）
	float backScareMultiplier = 1.5f;   // 背後から驚かせた時のゲージ上昇倍率
	float limitCos = cosf(XMConvertToRadians(backScareAngle / 2.0f));

	// 範囲内にいるバスターズのうち1体でも背後なら背後ボーナス適用
	bool isBackScare = false;
	int busterCount = Busters_GetCurrentFloorCount();
	for (int i = 0; i < busterCount; i++)
	{
		Busters* pBuster = GetBustersByIndex(i);
		if (!pBuster) continue;

		XMFLOAT3 busterPos = pBuster->GetPos();
		float dx = busterPos.x - ghostPos.x;
		float dz = busterPos.z - ghostPos.z;
		if (dx * dx + dz * dz > currentRange * currentRange) continue; // 範囲外はスキップ

		// バスターズの向いている角度から「正面」ベクトルを計算
		float rotRad = XMConvertToRadians(pBuster->GetRot().y + 180.0f);
		XMVECTOR forwardVec = XMVectorSet(sinf(rotRad), 0.0f, cosf(rotRad), 0.0f);

		// 「背後」のベクトル（正面の逆）
		XMVECTOR backwardVec = XMVectorSet(-XMVectorGetX(forwardVec), 0.0f, -XMVectorGetZ(forwardVec), 0.0f);

		// バスターズから幽霊(家具)への方向ベクトル
		XMVECTOR busterToGhost = XMVectorSet(ghostPos.x - busterPos.x, 0.0f, ghostPos.z - busterPos.z, 0.0f);
		busterToGhost = XMVector3Normalize(busterToGhost);

		// 内積で角度を判定（コサイン）
		float dot = XMVectorGetX(XMVector3Dot(backwardVec, busterToGhost));

		if (dot >= limitCos)
		{
			isBackScare = true;
			break;
		}
	}

	switch (action)
	{
	case ACTION_SCARE:
	{
		bool scared = false;

		if (Busters_IsAnyInRange(ghostPos, currentRange))
		{
			BustersScare();
			scared = true;
		}

		// チュートリアルバスターズが範囲内にいれば驚かせる
		TutorialBusters* pTutBuster = GetTutorialBusters();
		if (pTutBuster && pTutBuster->GetState() != TB_STUN)
		{
			XMFLOAT3 tutPos = pTutBuster->GetPos();
			XMVECTOR tutDistVec = XMVectorSubtract(XMLoadFloat3(&tutPos), XMLoadFloat3(&ghostPos));
			float tutDistance = XMVectorGetX(XMVector3Length(tutDistVec));
			if (tutDistance <= currentRange)
			{
				pTutBuster->OnScared();
				scared = true;
			}
		}

		if (scared)
		{
			if (!m_HasIncreasedMultiplier)
			{
				ScareComboUP();
				m_HasIncreasedMultiplier = true;
			}
			float addScore = SCORE_SCARE * UI_ScareCombo_GetNumber();
			
			if (isBackScare)
			{
				addScore *= backScareMultiplier; // 背後なら1.5倍にする
			}

			int busterCount = Busters_GetCurrentFloorCount();
			if (busterCount == 2) {
				addScore *= 0.8f; // 2人なら 80% にダウン
			}
			else if (busterCount >= 3) {
				addScore *= 0.6f; // 3人なら 60% にさらにダウン
			}
			addScore *= resistanceMult; // 家具耐性による減衰
			{
				float prevGauge = UI_GetScareGauge();
				AddScareGauge(addScore);
				float nextGauge = UI_GetScareGauge();
				float actualAdd = nextGauge - prevGauge;
			}
			// ゲージMAXの判定は Game_Update 内の通常ループで倒す
		}
		break;
	}

	case ACTION_LURE:
	{
		float lureRange = currentRange * 2.0f;

		if (Busters_IsAnyInRange(ghostPos, lureRange))
		{
			BustersLured(ghostPos, lureRange);
				if (!m_HasIncreasedMultiplier)
				{
					ScareComboUP();
					m_HasIncreasedMultiplier = true;
				}
			{
				float lureAdd = SCORE_LURE * UI_ScareCombo_GetNumber() * resistanceMult;
				float prevGauge = UI_GetScareGauge();
				AddScareGauge(lureAdd);
				float nextGauge = UI_GetScareGauge();
				float actualAdd = nextGauge - prevGauge;
			}
		}
		break;
	}

	case ACTION_STOP:

		if (Busters_IsAnyInRange(ghostPos, BUSTERS_STOP_RANGE))
		{
			BustersStopped();
				if (!m_HasIncreasedMultiplier)
				{
					ScareComboUP();
					m_HasIncreasedMultiplier = true;
				}
			{
				float stopAdd = SCORE_STOP * UI_ScareCombo_GetNumber() * resistanceMult;
				float prevGauge = UI_GetScareGauge();
				AddScareGauge(stopAdd);
				float nextGauge = UI_GetScareGauge();
				float actualAdd = nextGauge - prevGauge;
			}
		}
		break;
	}
}

void Ghost::FurnitureSearch(void)
{

	if (m_IsIlluminated)
	{
		m_InRangeFurnitureList.clear();
		m_InRangeFurnitureNum = -1;
		m_SelectedFurnitureListIndex = 0;
		this->SetState(GS_MOVING);
		return;
	}

	// バスターズがいない or フロア降下中は家具検知しない
	if (!HasAnyBusterOnCurrentFloor() || Game_IsFloorExitAnimActive())
	{
		ResetAllFurnitureGhostTarget();
		m_InRangeFurnitureList.clear();
		m_InRangeFurnitureNum = -1;
		m_SelectedFurnitureListIndex = 0;
		this->SetState(GS_MOVING);
		return;
	}

	// 範囲内にある全ての家具のインデックスをリストアップする
	std::vector<int> currentList;

	int furnitureCount = GetFurnitureCount();
	for (int i = 0; i < furnitureCount; i++)
	{
		Furniture* pFurniture = GetFurniture(i);
		if (pFurniture)
		{
			if (pFurniture->IsCoolingDown()) continue;
			if (pFurniture->GetActionType() == ACTION_NONE) continue;
			if (!TutorialObject_CanPossessFurnitureBlock(pFurniture->GetBlockID())) continue;

			// 範囲内にいればリストに追加
			if (pFurniture->GetDistanceToGhost() <= FURNITURE_DETECTION_RANGE)
			{
				currentList.push_back(i);
			}
		}
	}

	// リストが空（範囲内に家具がない）場合
	if (currentList.empty())
	{
		m_InRangeFurnitureList.clear();
		m_InRangeFurnitureNum = -1;
		m_SelectedFurnitureListIndex = 0;
		this->SetState(GS_MOVING);
	}
	else
	{
		// 最も近い家具を選択
		int nearestIndex = currentList[0];
		float nearestDist = FLT_MAX;
		for (size_t i = 0; i < currentList.size(); i++)
		{
			Furniture* pF = GetFurniture(currentList[i]);
			if (pF && pF->GetDistanceToGhost() < nearestDist)
			{
				nearestDist = pF->GetDistanceToGhost();
				nearestIndex = currentList[i];
			}
		}

		m_InRangeFurnitureList = currentList;
		m_InRangeFurnitureNum = nearestIndex;
		m_SelectedFurnitureListIndex = 0;

		Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
		if (pFurniture)
		{
			pFurniture->SetIsGhostTarget(true);
			this->SetState(GS_FURNITURE_FOUND);
		}
	}
}

void Ghost::Move(void)
{
	if (m_IsTransformed)
		return;

	float cameraYaw = Camera_GetYaw();
	float yawRad = XMConvertToRadians(cameraYaw);
	float forwardX = sinf(yawRad);
	float forwardZ = cosf(yawRad);
	float rightX = cosf(yawRad);
	float rightZ = -sinf(yawRad);

	XMVECTOR accelVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	if (Keyboard_IsKeyDown(KK_W)) accelVec = XMVectorAdd(accelVec, XMVectorSet(forwardX * GHOST_ACCELERATION, 0.0f, forwardZ * GHOST_ACCELERATION, 0.0f));
	if (Keyboard_IsKeyDown(KK_S)) accelVec = XMVectorAdd(accelVec, XMVectorSet(-forwardX * GHOST_ACCELERATION, 0.0f, -forwardZ * GHOST_ACCELERATION, 0.0f));
	if (Keyboard_IsKeyDown(KK_D)) accelVec = XMVectorAdd(accelVec, XMVectorSet(rightX * GHOST_ACCELERATION, 0.0f, rightZ * GHOST_ACCELERATION, 0.0f));
	if (Keyboard_IsKeyDown(KK_A)) accelVec = XMVectorAdd(accelVec, XMVectorSet(-rightX * GHOST_ACCELERATION, 0.0f, -rightZ * GHOST_ACCELERATION, 0.0f));

	XMVECTOR velocityVec = XMLoadFloat3(&m_Velocity);
	velocityVec = XMVectorAdd(velocityVec, accelVec);

	float speed = XMVectorGetX(XMVector3Length(velocityVec));
	if (speed > GHOST_MAX_SPEED)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_MAX_SPEED / speed);
	}

	if (XMVectorGetX(accelVec) == 0.0f && XMVectorGetY(accelVec) == 0.0f && XMVectorGetZ(accelVec) == 0.0f)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_DECELERATION);
	}

	XMStoreFloat3(&m_Velocity, velocityVec);

	float moveVecX = m_Velocity.x;
	float moveVecZ = m_Velocity.z;

	if (moveVecX != 0.0f || moveVecZ != 0.0f)
	{
		float moveAngle = atan2f(moveVecX, moveVecZ);
		float moveYaw = XMConvertToDegrees(moveAngle);
		SetRot({ 0.0f, moveYaw - 180.0f, 0.0f });
	}

	float r = 0.4f;

	float nextX = m_Position.x + m_Velocity.x;
	float hitX = false;

	if (Field_IsWallForGhost(nextX + r, m_Position.z + r) ||
		Field_IsWallForGhost(nextX + r, m_Position.z - r) ||
		Field_IsWallForGhost(nextX - r, m_Position.z + r) ||
		Field_IsWallForGhost(nextX - r, m_Position.z - r))
	{
		hitX = true;
	}

	if (hitX) m_Velocity.x = 0.0f;
	else m_Position.x = nextX;

	float nextZ = m_Position.z + m_Velocity.z;
	bool hitZ = false;

	if (Field_IsWallForGhost(m_Position.x + r, nextZ + r) ||
		Field_IsWallForGhost(m_Position.x + r, nextZ - r) ||
		Field_IsWallForGhost(m_Position.x - r, nextZ + r) ||
		Field_IsWallForGhost(m_Position.x - r, nextZ - r))
	{
		hitZ = true;
	}

	if (hitZ) m_Velocity.z = 0.0f;
	else m_Position.z = nextZ;

	SetPos(m_Position);
}

void Ghost::FloorMove(void)
{

	// 階段接近＋左クリックでの階層移動処理は無効化
	m_FloorCooldown = 0.0f;
	ResetColor();
}

void Ghost::ResetPos(void)
{
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	m_Position = { m_Position.x, GHOST_POS_Y, m_Position.z };
	m_InRangeFurnitureNum = -1;
	m_IsTransformed = false;
	m_HasIncreasedMultiplier = false;
}

// 変身中（GS_TRANSFORM / GS_SCARE）を強制解除してGS_MOVINGに戻す
// FLOOR_EXIT_OVERVIEW中など Ghost_Update() の外から呼ぶ用
void Ghost_ForceExitTransform(void)
{
	if (!g_Ghost) return;
	GHOST_STATE state = g_Ghost->GetState();
	if (state != GS_TRANSFORM && state != GS_SCARE) return;

	UpdateRangeCircleState();
	g_Ghost->ResetPos();

	// 解除後、壁や家具に埋まらない安全な隣接マスへ移動させる
	{
		XMFLOAT3 safePos = CalcSafeExitPos(g_Ghost->GetPos());
		g_Ghost->SetPos(safePos);
	}

	g_Ghost->SetState(GS_MOVING);
	g_Ghost->SetIsDraw(true);
}

Ghost* GetGhost(void)
{
	return g_Ghost;
}

XMFLOAT3 GetGhostStartPos(void)
{
	return GetGhostStartPos(START_FLOOR - 1);
}

XMFLOAT3 GetGhostStartPos(int floor)
{
	switch (floor)
	{
	case 0:
		return { GHOST_START_POS_FLOOR1_X, GHOST_POS_Y, GHOST_START_POS_FLOOR1_Z };
	case 1:
		return { GHOST_START_POS_FLOOR2_X, GHOST_POS_Y, GHOST_START_POS_FLOOR2_Z };
	case 2:
		return { GHOST_START_POS_FLOOR3_X, GHOST_POS_Y, GHOST_START_POS_FLOOR3_Z };
	default:
		return { GHOST_START_POS_FLOOR1_X, GHOST_POS_Y, GHOST_START_POS_FLOOR1_Z };
	}
}
