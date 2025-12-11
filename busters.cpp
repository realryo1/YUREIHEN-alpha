#include "busters.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"
#include "debug_ostream.h"
#include "define.h"
#include "field.h"
#include "furniture.h" // 家具の位置を知るために必要
#include <stdlib.h>    // rand()用

Busters* g_Busters = NULL;

// コンストラクタ
Busters::Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: Sprite3D(pos, scale, rot, pass),
	Jump(0.01f, 0.2f, PATROL_HEIGHT),
	m_State(BUSTERS_SEARCH),
	m_TargetFurnitureIndex(-1), // 最初は目標なし
	m_WaitTimer(0),
	m_MoveSpeed(0.03f), // 少しゆっくり探索
	m_DistanceToGhost(0.0f)
{
	// 乱数初期化（家具ランダム選択用）
	srand(GetTickCount());
}

void Busters::Update(void)
{
	// ジャンプ更新
	JumpUpdate(*(Transform3D*)this);

	// 状態の確認と遷移
	CheckState();

	XMFLOAT3 nextStepPos = m_Position; // 次に目指す一歩

	switch (m_State)
	{
	case BUSTERS_SEARCH: // 探索（経路移動）

		// 1. 目標がない場合、新しい家具を選んで経路計算
		if (m_TargetFurnitureIndex == -1)
		{
			if (m_WaitTimer > 0)
			{
				m_WaitTimer--;
				return;
			}

			// ランダムに家具を選ぶ
			m_TargetFurnitureIndex = rand() % FURNITURE_NUM;

			Furniture* targetFurniture = GetFurniture(m_TargetFurnitureIndex);
			if (targetFurniture)
			{
				//現在地から家具までの経路を計算してもらう
				m_PathList = Field_FindPath(m_Position, targetFurniture->GetPos());

				// 経路が見つからなかったらやり直し
				if (m_PathList.empty())
				{
					m_TargetFurnitureIndex = -1;
				}
			}
			else
			{
				m_TargetFurnitureIndex = -1;
			}
		}

		// 2. 経路に沿って移動
		if (!m_PathList.empty())
		{
			// リストの末尾が「次のマス」
			XMFLOAT3 targetNode = m_PathList.back();
			targetNode.y = m_Position.y; // 高さは合わせる

			// 次のマスに向かう
			nextStepPos = targetNode;

			// マスに到着したか判定（距離0.5以内）
			XMVECTOR myPosV = XMLoadFloat3(&m_Position);
			XMVECTOR targetV = XMLoadFloat3(&targetNode);
			if (XMVectorGetX(XMVector3Length(XMVectorSubtract(targetV, myPosV))) < 0.5f)
			{
				// 到着したのでリストから削除（次のマスへ）
				m_PathList.pop_back();
			}
		}
		else if (m_TargetFurnitureIndex != -1)
		{
			// 経路リストが空になった ＝ 最終目的地に到着
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 60; // 1秒キョロキョロ
		}

		m_MoveSpeed = 0.03f;
		break;

	case BUSTERS_CHASE: // 追跡（直線移動）
		nextStepPos = GetGhost()->GetPos();
		nextStepPos.y = m_Position.y;

		// 追跡に入ったら経路リストはクリアしておく
		m_PathList.clear();
		m_TargetFurnitureIndex = -1;

		m_MoveSpeed = 0.08f;
		break;
	}

	// 次の地点に向かって移動
	MoveTo(nextStepPos);
}
void Busters::CheckState(void)
{
	Ghost* ghost = GetGhost();
	if (!ghost) return;

	// Ghostとの距離を計算
	XMFLOAT3 ghostPos = ghost->GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR myVec = XMLoadFloat3(&m_Position);
	m_DistanceToGhost = XMVectorGetX(XMVector3Length(XMVectorSubtract(ghostVec, myVec)));

	// 視界判定
	bool isFound = false;

	// Ghostが変身していない、かつ一定距離内なら発見
	// ※変身中でも「移動した瞬間」などを見破るロジックを後で入れても面白い
	if (ghost->GetState() == GS_MOVING ||
		ghost->GetState() == GS_FURNITURE_FOUND)
	{
		if (m_DistanceToGhost < BUSTERS_PATROL_RANGH)
		{
			isFound = true;
		}
	}

	// 状態遷移
	if (isFound)
	{
		if (m_State != BUSTERS_CHASE)
		{
			m_State = BUSTERS_CHASE;
			this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤色（発見）
			ghost->SetIsDetectedByBuster(true);
		}
	}
	else
	{
		if (m_State == BUSTERS_CHASE)
		{
			// 見失った -> 探索に戻る
			m_State = BUSTERS_SEARCH;
			m_TargetFurnitureIndex = -1; // ターゲット再抽選
			m_WaitTimer = 30;            // 少し立ち止まって考える

			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
		}
	}
}

void Busters::MoveTo(XMFLOAT3 targetPos)
{
	// ジャンプ中は移動制御不能にするならここでreturn
	if (GetIsJumping()) return;

	// 方向ベクトル計算
	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;

	// 距離が近すぎるなら動かない
	if (fabsf(dx) < 0.1f && fabsf(dz) < 0.1f) return;

	// 正規化
	float len = sqrtf(dx * dx + dz * dz);
	if (len > 0)
	{
		dx /= len;
		dz /= len;
	}

	// 向きを変更（進行方向を向く）
	float angle = atan2f(dx, dz);
	float deg = XMConvertToDegrees(angle);
	SetRotY(deg + 180.0f); // モデルの向きに合わせて調整

	// ====================================================================
	// ★修正: サイズを考慮した壁判定 (めり込み防止)
	// ====================================================================

	// 当たり判定の半径 (ブロックサイズが1.0なので、0.3～0.4くらいが適切)
	float r = 0.4f;

	// --- X方向の移動 ---
	float nextX = m_Position.x + dx * m_MoveSpeed;
	bool hitX = false;

	// 移動先のX座標において、Z方向の幅(±r)を含めた4点をチェック
	// 壁(Y=0.0)があるか確認
	if (Field_IsWall(nextX + r, m_Position.y, m_Position.z + r) || // 右前
		Field_IsWall(nextX + r, m_Position.y, m_Position.z - r) || // 右後
		Field_IsWall(nextX - r, m_Position.y, m_Position.z + r) || // 左前
		Field_IsWall(nextX - r, m_Position.y, m_Position.z - r))   // 左後
	{
		hitX = true;
	}

	if (!hitX)
	{
		m_Position.x = nextX;
	}

	// --- Z方向の移動 ---
	float nextZ = m_Position.z + dz * m_MoveSpeed;
	bool hitZ = false;

	// X座標は更新後のものを使ってチェック
	if (Field_IsWall(m_Position.x + r, m_Position.y, nextZ + r) || // 右前
		Field_IsWall(m_Position.x + r, m_Position.y, nextZ - r) || // 右後
		Field_IsWall(m_Position.x - r, m_Position.y, nextZ + r) || // 左前
		Field_IsWall(m_Position.x - r, m_Position.y, nextZ - r))   // 左後
	{
		hitZ = true;
	}

	if (!hitZ)
	{
		m_Position.z = nextZ;
	}
}
void Busters::OnScared(void)
{
	JumpStart();
	// 驚いたときはターゲットをリセットして一時停止
	m_TargetFurnitureIndex = -1;
	m_WaitTimer = 120; // 2秒間動けなくする
	this->SetColor(0.0f, 0.0f, 1.0f, 1.0f); // 青色（驚いた）
}

void Busters::SetIsGhostDiscover(bool discover)
{
	// trueならMaterial色を緑にする
	if (discover)
	{
		this->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		this->ResetColor();
	}
}

// --- グローバル関数 ---

void Busters_Initialize(void)
{
	// 初期位置 (右下の部屋あたりからスタート)
	g_Busters = new Busters(
		{ 10.0f, PATROL_HEIGHT, 10.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\buster.fbx"
	);

	// 着地高さ設定
	if (g_Busters)
	{
		g_Busters->SetGroundLevel(PATROL_HEIGHT);
	}
}

// ... (以下変更なし) ...
void Busters_Update(void)
{
	if (g_Busters) g_Busters->Update();
}

void Busters_Draw(void)
{
	if (g_Busters) g_Busters->Draw();
}

void Busters_Finalize(void)
{
	if (g_Busters) {
		delete g_Busters;
		g_Busters = NULL;
	}
}

Busters* GetBusters(void)
{
	return g_Busters;
}

void BustersScare(void)
{
	if (g_Busters) g_Busters->OnScared();
}