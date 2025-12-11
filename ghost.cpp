#include "ghost.h"
#include "main.h"
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
#include "UI.h"
#include "UI_scarecombo.h"
#include "define.h"

Ghost* g_Ghost = NULL;

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_Ghost = new Ghost(
		{ -3.0f, Ghost::GetGhostPosY(), -10.0f },	//位置　終わってる初期化
		{ 1.0f, 1.0f, 1.0f },					//スケール
		{ 0.0f, 180.0f, 0.0f },					//回転（度）
		"asset\\model\\ghost.fbx"				//モデルパス
	);
}

void Ghost_Update(void)
{
	if (!g_Ghost) return;

	switch (g_Ghost->GetState())
	{
	case GS_MOVING:
		g_Ghost->SetIsDraw(true);		// 描画有効化
		g_Ghost->Move();				// 移動処理
		g_Ghost->FurnitureSearch();		// 近くの家具検知と色変更
		g_Ghost->FloorMove();			// 階段移動処理
		break;
	case GS_FURNITURE_FOUND:
		g_Ghost->SetIsDraw(true);		// 描画有効化
		g_Ghost->Move();				// 移動処理
		g_Ghost->FurnitureSearch();		// 近くの家具検知と色変更
		g_Ghost->FloorMove();			// 階段移動処理

		// 変身開始処理
		if (Keyboard_IsKeyDownTrigger(KK_E))
		{
			g_Ghost->SetState(GS_TRANSFORM);
		}

		break;
	case GS_TRANSFORM:
		g_Ghost->SetIsDraw(false);		// 描画有効化
		g_Ghost->Transforming();	// 変身中処理

		//spaceで驚かせるアクション
		if (Keyboard_IsKeyDownTrigger(KK_SPACE))
		{
			g_Ghost->SetState(GS_SCARE);

			//家具とプレイヤーをジャンプさせる(ここで呼ぶのキモい)
			g_Ghost->ScareStart();
		}

		// 変身解除処理
		if (Keyboard_IsKeyDownTrigger(KK_E))
		{
			g_Ghost->ResetPos();
			g_Ghost->SetState(GS_MOVING);

		}
		break;
	case GS_SCARE:
		g_Ghost->SetIsDraw(false);		// 描画有効化
		g_Ghost->Transforming();	// 変身中処理
		//家具のジャンプが終わったらTransFormに戻る
		if (FurnitureScareEnded(g_Ghost->GetInRangeNum()))
		{
			g_Ghost->SetState(GS_TRANSFORM);
		}
		break;
	default:
		break;
	}

	// カメラの注視対象をGhost位置に設定
	Camera_SetTargetPos(g_Ghost->GetPos());

	// ステート処理をデバッグ出力
	hal::dout << "Ghost State: " << g_Ghost->GetState() << std::endl;
}

void Ghost_Draw(void)
{
	g_Ghost->Draw();
}

void Ghost_Finalize(void)
{
	if (g_Ghost)
	{
		delete g_Ghost;
		g_Ghost = NULL;
	}
}

// ========== Ghost クラスメソッドの実装 ==========

//void Ghost::Transforming(void)
//{
//	 m_IsDetectedByBusterがtrueの場合かつ変身中でないとき、1秒につきマイナス1する
//	if (m_IsDetectedByBuster && !m_IsTransformed)
//	{
//		m_DetectionTimer += 1.0f / 60.0f;  // 1フレームの時間を加算（60FPS想定）
//
//		if (m_DetectionTimer >= 0.5f)
//		{
//			AddScareGauge(-2.0f);  // マイナス1を恐怖ゲージに加算
//			m_DetectionTimer -= 0.5f;  // 1秒分を引く
//		}
//	}
//	else
//	{
//		m_DetectionTimer = 0.0f;  // フラグがfalseになったらタイマーをリセット
//	}
//}
//
void Ghost::Transforming(void)
{
	// Ghostを家具の位置に合わせる
	Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
	if (pFurniture)
	{
		SetPos(pFurniture->GetPos());
	}

	// Ghost（Furniture） と buster の距離を計算
	XMFLOAT3 busterPos = GetBusters()->GetPos();
	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR busterVec = XMLoadFloat3(&busterPos);
	XMVECTOR distVec = XMVectorSubtract(busterVec, ghostVec);
	float distance = XMVectorGetX(XMVector3Length(distVec));

	//距離が一定以下なら驚かせる
	if (distance <= SCARE_RANGE)
	{
		//レンジに入っているなら色を変える
		GetBusters()->SetIsGhostDiscover(true);
	}
	else
	{
		GetBusters()->SetIsGhostDiscover(false);
	}
}

void Ghost::ScareStart(void)
{
	FurnitureScareStart(m_InRangeFurnitureNum);

	// Ghost（Furniture） と buster の距離を計算
	XMFLOAT3 busterPos = GetBusters()->GetPos();
	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR busterVec = XMLoadFloat3(&busterPos);
	XMVECTOR distVec = XMVectorSubtract(busterVec, ghostVec);
	float distance = XMVectorGetX(XMVector3Length(distVec));

	//距離が一定以下なら驚かせる
	if (distance <= SCARE_RANGE)
	{
		BustersScare();			// 
		ScareComboUP();			//恐怖コンボを上げる
		AddScareGauge(1.0f * UI_ScareCombo_GetNumber());			// 恐怖ゲージ加算
	}
}

void Ghost::FurnitureSearch(void)
{
	float tempDistance = 999999.0f;
	int tempInRangeNum = -1;

	// Ghost最近傍のFurnitureを探索
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		Furniture* pFurniture = GetFurniture(i);
		if (pFurniture)
		{
			pFurniture->ResetColor();  // 元の色に戻す

			// 距離が検出範囲内かつ一時保存された家具との距離より近い場合
			if (pFurniture->GetDistanceToGhost() <= FURNITURE_DETECTION_RANGE &&
				pFurniture->GetDistanceToGhost() < tempDistance)
			{
				tempDistance = pFurniture->GetDistanceToGhost();
				tempInRangeNum = i;
			}
		}
	}

	// 最も近い家具を変身対象とする
	if (tempInRangeNum != -1)
	{
		m_InRangeFurnitureNum = tempInRangeNum;

		//検出範囲内の家具がある場合、その家具の色を変える
		Furniture* pFurniture = GetFurniture(m_InRangeFurnitureNum);
		if (pFurniture)
		{
			pFurniture->SetColor(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
			this->SetState(GS_FURNITURE_FOUND);
		}
	}
	else
	{
		this->SetState(GS_MOVING);
	}
}

void Ghost::Move(void)
{
	// 変身していないときのみ移動可能
	if (m_IsTransformed)
		return;

	// カメラのヨー角を取得
	float cameraYaw = Camera_GetYaw();
	float yawRad = XMConvertToRadians(cameraYaw);

	// カメラの向きに基づいた方向ベクトル
	float forwardX = sinf(yawRad);
	float forwardZ = cosf(yawRad);
	float rightX = cosf(yawRad);
	float rightZ = -sinf(yawRad);

	// 入力による加速度ベクトル
	XMVECTOR accelVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	// W: 前方移動（カメラの向き）
	if (Keyboard_IsKeyDown(KK_W))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(forwardX * GHOST_ACCELERATION, 0.0f, forwardZ * GHOST_ACCELERATION, 0.0f));
	}
	// S: 後方移動
	if (Keyboard_IsKeyDown(KK_S))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(-forwardX * GHOST_ACCELERATION, 0.0f, -forwardZ * GHOST_ACCELERATION, 0.0f));
	}
	// D: 右移動
	if (Keyboard_IsKeyDown(KK_D))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(rightX * GHOST_ACCELERATION, 0.0f, rightZ * GHOST_ACCELERATION, 0.0f));
	}
	// A: 左移動
	if (Keyboard_IsKeyDown(KK_A))
	{
		accelVec = XMVectorAdd(accelVec, XMVectorSet(-rightX * GHOST_ACCELERATION, 0.0f, -rightZ * GHOST_ACCELERATION, 0.0f));
	}

	// 速度を更新（加速度を適用）
	XMVECTOR velocityVec = XMLoadFloat3(&m_Velocity);
	velocityVec = XMVectorAdd(velocityVec, accelVec);

	// 速度の大きさを制限
	float speed = XMVectorGetX(XMVector3Length(velocityVec));
	if (speed > GHOST_MAX_SPEED)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_MAX_SPEED / speed);
	}

	// 入力がない場合は減速
	if (XMVectorGetX(accelVec) == 0.0f && XMVectorGetY(accelVec) == 0.0f && XMVectorGetZ(accelVec) == 0.0f)
	{
		velocityVec = XMVectorScale(velocityVec, GHOST_DECELERATION);
	}

	XMStoreFloat3(&m_Velocity, velocityVec);

	// 水平方向（XZ）の移動ベクトルから Ghost の向きを決定
	float moveVecX = m_Velocity.x;
	float moveVecZ = m_Velocity.z;

	// 水平方向の速度が0でない場合、その方向を向く
	if (moveVecX != 0.0f || moveVecZ != 0.0f)
	{
		float moveAngle = atan2f(moveVecX, moveVecZ);
		float moveYaw = XMConvertToDegrees(moveAngle);
		SetRot({ 0.0f, moveYaw - 180.0f, 0.0f });
	}

	// Ghost位置を更新
	XMFLOAT3 ghostPos = GetPos();
	XMVECTOR posVec = XMLoadFloat3(&ghostPos);
	posVec = XMVectorAdd(posVec, velocityVec);
	XMFLOAT3 newPos;
	XMStoreFloat3(&newPos, posVec);
	SetPos(newPos);
}

void Ghost::FloorMove(void)
{
	// 1. クールダウンタイマーの更新
	if (m_FloorCooldown > 0.0f)
	{
		m_FloorCooldown -= 1.0f / 60.0f;
	}


	if (m_FloorCooldown <= 0.0f)
	{
		FIELD_TYPE blockType = Field_GetBlockType(m_Position.x, m_Position.z);

		// 階段の上にいる場合のみマウス判定を行う
		if (blockType == FIELD_STAIRS_UP || blockType == FIELD_STAIRS_DOWN)
		{
			// マウスの状態を取得
			Mouse_State mouseState;
			Mouse_GetState(&mouseState);

			// 左クリックが押されているなら移動実行
			if (mouseState.leftButton)
			{
				if (blockType == FIELD_STAIRS_UP)
				{
					int currentFloor = Field_GetCurrentFloor();
					Field_ChangeFloor(currentFloor + 1);

					// 位置ずらし + クールダウン設定
					m_Position.z += 2.0f;
					SetPos(m_Position);
					m_FloorCooldown = 2.0f; // 2秒間移動禁止
				}
				else if (blockType == FIELD_STAIRS_DOWN)
				{
					int currentFloor = Field_GetCurrentFloor();
					Field_ChangeFloor(currentFloor - 1);

					// 位置ずらし + クールダウン設定
					m_Position.z -= 2.0f;
					SetPos(m_Position);
					m_FloorCooldown = 2.0f; // 2秒間移動禁止
				}
			}
		}
	}
}

void Ghost::ResetPos(void)
{
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	m_Position = { m_Position.x, GHOST_POS_Y, m_Position.z };
	m_InRangeFurnitureNum = -1;
	m_IsTransformed = false;
}

//ghostのゲッター
Ghost* GetGhost(void)
{
	return g_Ghost;
}