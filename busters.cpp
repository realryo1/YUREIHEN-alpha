#include "busters.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"
#include "debug_ostream.h"
#include "define.h"
#include "field.h"
#include "furniture.h"
#include <stdlib.h>

// 階層分のBustersリスト
static Busters* g_BustersList[MAP_FLOORS];

// =================================================================
// Busters クラスメンバ関数の実装
// =================================================================

// コンストラクタ
Busters::Busters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: Sprite3D(pos, scale, rot, pass),
	Jump(0.01f, 0.2f, PATROL_HEIGHT),
	m_State(BUSTERS_SEARCH),
	m_TargetFurnitureIndex(-1),
	m_WaitTimer(0),
	m_Velocity(0.0f, 0.0f, 0.0f),
	m_MoveSpeed(0.03f),
	m_DistanceToGhost(0.0f)
{
	// 乱数初期化 (警告対策: キャストを入れる)
	srand((unsigned int)GetTickCount64());
}

// 更新処理
void Busters::Update(void)
{
	JumpUpdate(*(Transform3D*)this);
	CheckState();

	XMFLOAT3 nextStepPos = m_Position;

	switch (m_State)
	{
	case BUSTERS_SEARCH: // 探索
		if (m_TargetFurnitureIndex == -1)
		{
			if (m_WaitTimer > 0)
			{
				m_WaitTimer--;
				return;
			}
			m_TargetFurnitureIndex = rand() % FURNITURE_NUM;
			Furniture* targetFurniture = GetFurniture(m_TargetFurnitureIndex);
			if (targetFurniture)
			{
				m_PathList = Field_FindPath(m_Position, targetFurniture->GetPos());
				if (m_PathList.empty()) m_TargetFurnitureIndex = -1;
			}
			else
			{
				m_TargetFurnitureIndex = -1;
			}
		}

		if (!m_PathList.empty())
		{
			XMFLOAT3 targetNode = m_PathList.back();
			targetNode.y = m_Position.y;
			nextStepPos = targetNode;
			XMVECTOR myPosV = XMLoadFloat3(&m_Position);
			XMVECTOR targetV = XMLoadFloat3(&targetNode);
			if (XMVectorGetX(XMVector3Length(XMVectorSubtract(targetV, myPosV))) < 0.5f)
			{
				m_PathList.pop_back();
			}
		}
		else if (m_TargetFurnitureIndex != -1)
		{
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 60;
		}

		m_MoveSpeed = 0.03f; // 通常速度
		break;

	case BUSTERS_SUSPICION: // 警戒（中距離）
		m_PathList.clear();
		m_TargetFurnitureIndex = -1;

		nextStepPos = GetGhost()->GetPos();
		nextStepPos.y = m_Position.y;

		m_MoveSpeed = 0.06f; // 少し早歩き
		break;

	case BUSTERS_CHASE: // 追跡（近距離）
		nextStepPos = GetGhost()->GetPos();
		nextStepPos.y = m_Position.y;

		m_PathList.clear();
		m_TargetFurnitureIndex = -1;

		m_MoveSpeed = 0.09f; // 全力疾走
		break;
	}

	MoveTo(nextStepPos);
}

// 状態遷移チェック
// busters.cpp の CheckState 関数

void Busters::CheckState(void)
{
	Ghost* ghost = GetGhost();
	if (!ghost) return;

	XMFLOAT3 ghostPos = ghost->GetPos();
	XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
	XMVECTOR myVec = XMLoadFloat3(&m_Position);
	m_DistanceToGhost = XMVectorGetX(XMVector3Length(XMVectorSubtract(ghostVec, myVec)));

	// 変身中は気づかない
	if (ghost->GetState() == GS_TRANSFORM ||
		ghost->GetState() == GS_SCARE)
	{
		if (m_State != BUSTERS_SEARCH)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor();
			ghost->SetIsDetectedByBuster(false);
			m_WaitTimer = 60;
		}

		return;
	}

	// ★追加: 間に壁があるかチェック (視線が通っているか？)
	bool hasWall = Field_CheckWallBetween(m_Position, ghostPos);

	// 距離による判定 (壁がない場合のみ検知)
	if (!hasWall && m_DistanceToGhost < BUSTERS_PATROL_RANGH) // 近距離
	{
		if (m_State != BUSTERS_CHASE)
		{
			m_State = BUSTERS_CHASE;
			this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤
			ghost->SetIsDetectedByBuster(true);
		}
	}
	else if (!hasWall && m_DistanceToGhost < BUSTERS_SUSPICION_RANGE) // 中距離
	{
		if (m_State != BUSTERS_SUSPICION)
		{
			m_State = BUSTERS_SUSPICION;
			this->SetColor(1.0f, 1.0f, 0.0f, 1.0f); // 黄
			ghost->SetIsDetectedByBuster(false);
		}
	}
	else // 範囲外、または壁がある
	{
		if (m_State != BUSTERS_SEARCH)
		{
			m_State = BUSTERS_SEARCH;
			this->ResetColor(); // 白
			ghost->SetIsDetectedByBuster(false);

			// 見失ったので少しキョロキョロさせる演出
			m_TargetFurnitureIndex = -1;
			m_WaitTimer = 30;
		}
	}
}
// 移動処理（壁判定あり）
void Busters::MoveTo(XMFLOAT3 targetPos)
{
	if (GetIsJumping()) return;

	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;

	if (fabsf(dx) < 0.1f && fabsf(dz) < 0.1f) return;

	float len = sqrtf(dx * dx + dz * dz);
	if (len > 0)
	{
		dx /= len;
		dz /= len;
	}

	float angle = atan2f(dx, dz);
	float deg = XMConvertToDegrees(angle);
	SetRotY(deg + 180.0f);

	float r = 0.4f;

	// X方向
	float nextX = m_Position.x + dx * m_MoveSpeed;
	bool hitX = false;
	if (Field_IsWall(nextX + r, m_Position.y, m_Position.z + r) ||
		Field_IsWall(nextX + r, m_Position.y, m_Position.z - r) ||
		Field_IsWall(nextX - r, m_Position.y, m_Position.z + r) ||
		Field_IsWall(nextX - r, m_Position.y, m_Position.z - r))
	{
		hitX = true;
	}
	if (!hitX) m_Position.x = nextX;

	// Z方向
	float nextZ = m_Position.z + dz * m_MoveSpeed;
	bool hitZ = false;
	if (Field_IsWall(m_Position.x + r, m_Position.y, nextZ + r) ||
		Field_IsWall(m_Position.x + r, m_Position.y, nextZ - r) ||
		Field_IsWall(m_Position.x - r, m_Position.y, nextZ + r) ||
		Field_IsWall(m_Position.x - r, m_Position.y, nextZ - r))
	{
		hitZ = true;
	}
	if (!hitZ) m_Position.z = nextZ;
}

void Busters::OnScared(void)
{
	JumpStart();
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

// =================================================================
// グローバル関数
// =================================================================

void Busters_Initialize(void)
{
	// 1階
	g_BustersList[0] = new Busters({ 0.0f, PATROL_HEIGHT, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\buster.fbx");
	if (g_BustersList[0]) g_BustersList[0]->SetGroundLevel(PATROL_HEIGHT);
	// 2階
	g_BustersList[1] = new Busters({ -10.0f, PATROL_HEIGHT, 10.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\buster.fbx");
	if (g_BustersList[1]) g_BustersList[1]->SetGroundLevel(PATROL_HEIGHT);
	// 3階
	g_BustersList[2] = new Busters({ 10.0f, PATROL_HEIGHT, -10.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, "asset\\model\\buster.fbx");
	if (g_BustersList[2]) g_BustersList[2]->SetGroundLevel(PATROL_HEIGHT);
}

void Busters_Update(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS && g_BustersList[currentFloor])
		g_BustersList[currentFloor]->Update();
}

void Busters_Draw(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS && g_BustersList[currentFloor])
		g_BustersList[currentFloor]->Draw();
}

void Busters_Finalize(void)
{
	for (int i = 0; i < MAP_FLOORS; i++)
	{
		if (g_BustersList[i]) { delete g_BustersList[i]; g_BustersList[i] = NULL; }
	}
}

Busters* GetBusters(void)
{
	int currentFloor = Field_GetCurrentFloor();
	if (currentFloor >= 0 && currentFloor < MAP_FLOORS) return g_BustersList[currentFloor];
	return NULL;
}

void BustersScare(void)
{
	Busters* target = GetBusters();
	if (target) target->OnScared();
}