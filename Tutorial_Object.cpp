#pragma execution_character_set("utf-8")
#include <cmath>
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;
#include "Tutorial_Object.h"
#include "sprite3d.h"
#include "field.h"
#include "ghost.h"
#include "UI_Tutorial.h"
#include "furniture.h"
#include "shader.h"
#include "define.h"
#include "keyboard.h"
#include "light.h"
#include "camera.h"

// ==========================================
// 円盤（enban）Sprite3D
// ==========================================
static Sprite3D* g_pEnban      = nullptr;
static bool      g_EnbanTouched   = false;
static bool      g_EnbanVisible   = false;
static bool      g_PianoPossessed = false;
static bool      g_BustersVisible = false;
static bool      g_BustersStunned = false; // バスターズをスタンさせたフラグ
static bool      g_PossessionOnlyPiano = false;
static bool      g_ScareEnabled = true;
static bool      g_ScareRequireBusterInRange = false;

// =================================================================
// グローバル変数
// =================================================================
static TutorialBusters* g_pTutorialBusters = nullptr;
static TutorialMarker*  g_pTutorialMarker  = nullptr;

// =================================================================
// TutorialMarker クラスメンバ関数の実装
// =================================================================

TutorialMarker::TutorialMarker()
	: m_BasePos(0.0f, 0.0f, 0.0f)
	, m_Arrow(nullptr)
	, m_ScreenArrow(nullptr)
	, m_BobTimer(0.0f)
	, m_Visible(true)
	, m_UseScreenArrow(false)
	, m_ScreenArrowPos(0.0f, 0.0f)
	, m_ScreenArrowRot(0.0f)
{
}

TutorialMarker::~TutorialMarker()
{
	if (m_Arrow)
	{
		delete m_Arrow;
		m_Arrow = nullptr;
	}
	for (Billboard* b : m_Arrows)
		delete b;
	m_Arrows.clear();

	if (m_ScreenArrow)
	{
		delete m_ScreenArrow;
		m_ScreenArrow = nullptr;
	}
}

void TutorialMarker::Initialize(const XMFLOAT3& pos)
{
	m_BasePos  = pos;
	m_BobTimer = 0.0f;
	m_Visible  = false;
	m_UseScreenArrow = false;

	m_Arrow = new Billboard();
	m_Arrow->Initialize(
		{ m_BasePos.x, m_BasePos.y + TUTORIAL_MARKER_BASE_HEIGHT, m_BasePos.z },
		{ TUTORIAL_MARKER_SIZE, TUTORIAL_MARKER_SIZE },
		{ 0.0f, 0.0f, 0.0f },
		true
	);
	m_Arrow->SetIcon(BILLBOARD_ICON::DESTINATION);

	m_ScreenArrow = new Sprite(
		{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
		{ 160.0f, 160.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\icon_shitayazirusi.png"
	);
}

// 複数座標対応：m_PosList を元にBillboard配列を作り直す
void TutorialMarker::RebuildArrows(void)
{
	for (Billboard* b : m_Arrows)
		delete b;
	m_Arrows.clear();

	for (const XMFLOAT3& pos : m_PosList)
	{
		Billboard* b = new Billboard();
		b->Initialize(
			{ pos.x, pos.y + TUTORIAL_MARKER_BASE_HEIGHT, pos.z },
			{ TUTORIAL_MARKER_SIZE, TUTORIAL_MARKER_SIZE },
			{ 0.0f, 0.0f, 0.0f },
			true
		);
		b->SetIcon(BILLBOARD_ICON::DESTINATION);
		m_Arrows.push_back(b);
	}
}

void TutorialMarker::SetPos(const XMFLOAT3& pos)
{
	m_BasePos = pos;
	// 複数座標モードをクリアして単一座標に戻す
	m_PosList.clear();
	m_PosList.push_back(pos);
	RebuildArrows();
}

void TutorialMarker::SetPositions(const std::vector<XMFLOAT3>& positions)
{
	m_PosList = positions;
	if (!m_PosList.empty())
		m_BasePos = m_PosList[0];
	RebuildArrows();
}

void TutorialMarker::Update(void)
{
	// 使用するリスト（複数優先）
	const bool useMulti = !m_PosList.empty() && !m_Arrows.empty();

	if (!m_Visible) return;
	if (!useMulti && !m_Arrow) return;

	const float dt = 1.0f / 60.0f;
	m_BobTimer += TUTORIAL_MARKER_BOB_SPEED * dt;
	float offsetY = sinf(m_BobTimer) * TUTORIAL_MARKER_BOB_AMP;

	if (useMulti)
	{
		for (size_t i = 0; i < m_PosList.size() && i < m_Arrows.size(); ++i)
		{
			XMFLOAT3 arrowPos = {
				m_PosList[i].x,
				m_PosList[i].y + TUTORIAL_MARKER_BASE_HEIGHT + offsetY,
				m_PosList[i].z
			};
			m_Arrows[i]->SetPos(arrowPos);
			m_Arrows[i]->Update();
		}
	}
	else
	{
		XMFLOAT3 arrowPos = {
			m_BasePos.x,
			m_BasePos.y + TUTORIAL_MARKER_BASE_HEIGHT + offsetY,
			m_BasePos.z
		};
		m_Arrow->SetPos(arrowPos);
		m_Arrow->Update();
	}

	m_UseScreenArrow = false;

	Camera* cam = GetCamera();
	if (!cam || !m_ScreenArrow) return;

	XMMATRIX view = cam->GetView();
	XMMATRIX proj = cam->GetProjection();
	XMMATRIX viewProj = view * proj;

	// 2D矢印：最も近い座標（画面外のもの）を選ぶ
	Ghost* ghost = GetGhost();
	XMFLOAT3 ghostPos = ghost ? ghost->GetPos() : XMFLOAT3(0.0f, 0.0f, 0.0f);

	// 全座標を確認して「画面外かつ最もゴーストに近い」ものを選ぶ
	float bestDistSq = -1.0f;
	int bestIdx = -1;

	auto checkPos = [&](int idx, const XMFLOAT3& pos) {
		XMVECTOR posVec = XMVectorSet(pos.x, pos.y + TUTORIAL_MARKER_BASE_HEIGHT, pos.z, 1.0f);
		XMVECTOR clipPos = XMVector3TransformCoord(posVec, viewProj);
		XMFLOAT3 ndc;
		XMStoreFloat3(&ndc, clipPos);

		bool isBehind = (ndc.z < 0.0f || ndc.z > 1.0f);
		bool outOfScreen = (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f);

		if (isBehind || outOfScreen)
		{
			float dx = pos.x - ghostPos.x;
			float dz = pos.z - ghostPos.z;
			float distSq = dx * dx + dz * dz;
			if (bestIdx == -1 || distSq < bestDistSq)
			{
				bestDistSq = distSq;
				bestIdx = idx;
			}
		}
	};

	if (useMulti)
	{
		for (int i = 0; i < (int)m_PosList.size(); ++i)
			checkPos(i, m_PosList[i]);
	}
	else
	{
		checkPos(0, m_BasePos);
	}

	if (bestIdx >= 0)
	{
		const XMFLOAT3& targetPos = useMulti ? m_PosList[bestIdx] : m_BasePos;

		XMVECTOR posVec = XMVectorSet(targetPos.x, targetPos.y + TUTORIAL_MARKER_BASE_HEIGHT, targetPos.z, 1.0f);
		XMVECTOR clipPos = XMVector3TransformCoord(posVec, viewProj);
		XMFLOAT3 ndc;
		XMStoreFloat3(&ndc, clipPos);

		bool isBehind = (ndc.z < 0.0f || ndc.z > 1.0f);

		m_UseScreenArrow = true;

		float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
		float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;

		float cx = SCREEN_WIDTH * 0.5f;
		float cy = SCREEN_HEIGHT * 0.5f;
		float vx = screenX - cx;
		float vy = screenY - cy;

		if (isBehind)
		{
			vx = -vx;
			vy = -vy;
		}

		if (fabsf(vx) < 0.001f && fabsf(vy) < 0.001f)
		{
			vy = -1.0f;
		}

		const float margin = 110.0f;
		float halfW = SCREEN_WIDTH * 0.5f - margin;
		float halfH = SCREEN_HEIGHT * 0.5f - margin;

		float tx = (fabsf(vx) > 0.001f) ? (halfW / fabsf(vx)) : 99999.0f;
		float ty = (fabsf(vy) > 0.001f) ? (halfH / fabsf(vy)) : 99999.0f;
		float t = (tx < ty) ? tx : ty;

		m_ScreenArrowPos = { cx + vx * t, cy + vy * t };
		m_ScreenArrowRot = XMConvertToDegrees(atan2f(vy, vx)) + 270.0f;

		m_ScreenArrow->SetPos(m_ScreenArrowPos);
		m_ScreenArrow->SetRot(m_ScreenArrowRot);
	}
}

void TutorialMarker::Draw(void)
{
	if (!m_Visible) return;

	const bool useMulti = !m_PosList.empty() && !m_Arrows.empty();

	SetDepthTest(false);
	Shader_Begin();

	if (useMulti)
	{
		for (size_t i = 0; i < m_Arrows.size(); ++i)
			m_Arrows[i]->Draw();
	}
	else
	{
		if (m_Arrow && !m_UseScreenArrow)
			m_Arrow->Draw();
	}

	SetDepthTest(true);
}

void TutorialMarker::Draw2D(void)
{
	if (!m_Visible || !m_UseScreenArrow || !m_ScreenArrow) return;
	m_ScreenArrow->Draw();
}

// =================================================================
// TutorialBusters クラスメンバ関数の実装
// =================================================================

TutorialBusters::TutorialBusters(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
	: AnimSprite3D(pos, scale, rot, pass)
	, Jump(0.01f, 0.2f, BUSTERS_HEIGHT)
	, m_State(TB_IDLE)
	, m_Icon(nullptr)
	, m_pHeadlight(nullptr)
	, m_MoveSpeed(BUSTERS_MOVE_SPEED_SEARCH)
	, m_WaitTimer(0)
	, m_KeepStateTimer(0)
	, m_HasTarget(false)
	, m_TargetPos(0.0f, 0.0f, 0.0f)
	, m_IsExiting(false)
	, m_ExitTargetPos(0.0f, 0.0f, 0.0f)
{
	m_Icon = new Billboard();
	m_Icon->Initialize({ 0.0f, 0.0f, 0.0f }, { 0.7f, 0.7f }, { 0.0f, 0.0f, 0.0f }, true);

	m_pHeadlight = new PointLight(
		TRUE,
		XMFLOAT4(pos.x, pos.y + 1.6f, pos.z, 1.0f),
		XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f),
		XMFLOAT4(1.0f, 1.0f, 0.95f, 1.0f),
		10.0f,
		2.0f
	);
}

TutorialBusters::~TutorialBusters()
{
	if (m_Icon)
	{
		delete m_Icon;
		m_Icon = nullptr;
	}
	if (m_pHeadlight)
	{
		delete m_pHeadlight;
		m_pHeadlight = nullptr;
	}
}

// -------------------------------------------------------
// 視野判定（Busters::IsTargetInFOV と同ロジック）
// -------------------------------------------------------
bool TutorialBusters::IsTargetInFOV(const XMFLOAT3& targetPos, float range) const
{
	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;
	float distSq = dx * dx + dz * dz;

	if (distSq > range * range) return false;

	float rotRad = XMConvertToRadians(GetRot().y + 180.0f);
	XMVECTOR forwardVec = XMVectorSet(sinf(rotRad), 0.0f, cosf(rotRad), 0.0f);

	XMVECTOR dirVec = XMVector3Normalize(XMVectorSet(dx, 0.0f, dz, 0.0f));
	float dot = XMVectorGetX(XMVector3Dot(forwardVec, dirVec));
	float limitCos = cosf(XMConvertToRadians(BUSTERS_FOV_ANGLE / 2.0f));

	return dot >= limitCos;
}

// -------------------------------------------------------
// 状態遷移チェック（Busters::CheckState の簡略版）
// -------------------------------------------------------
void TutorialBusters::CheckState(void)
{
	if (m_State == TB_STUN) return;

	Ghost* ghost = GetGhost();
	if (!ghost) return;

	// 変身中・驚かせ中は見つからない
	if (ghost->GetState() == GS_TRANSFORM || ghost->GetState() == GS_SCARE)
	{
		if (m_State == TB_CHASE)
			SetState(TB_SUSPICION);
		return;
	}

	bool hasWall = Field_CheckWallBetween(m_Position, ghost->GetPos());

	float chaseRange     = BUSTERS_PATROL_RANGH;
	float suspicionRange = BUSTERS_SUSPICION_RANGE;
	if (m_State == TB_CHASE)     chaseRange     *= 1.2f;
	if (m_State == TB_SUSPICION) suspicionRange *= 1.2f;

	bool inChase     = !hasWall && IsTargetInFOV(ghost->GetPos(), chaseRange);
	bool inSuspicion = !hasWall && IsTargetInFOV(ghost->GetPos(), suspicionRange);

	if (inChase)
	{
		m_KeepStateTimer = KEEP_STATE_TIME;
		if (m_State != TB_CHASE)
		{
			SetState(TB_CHASE);
			if (m_WaitTimer <= 0)
				m_WaitTimer = WAIT_TIMER_DEFAULT;
		}
	}
	else if (inSuspicion)
	{
		m_KeepStateTimer = KEEP_STATE_TIME;
		if (m_State != TB_SUSPICION)
		{
			SetState(TB_SUSPICION);
			if (m_WaitTimer <= 0)
				m_WaitTimer = WAIT_TIMER_DEFAULT;
		}
	}
	else
	{
		if (m_KeepStateTimer > 0)
		{
			m_KeepStateTimer--;
		}
		else
		{
			// 調査対象がある場合は IDLE に戻さず SUSPICION のまま維持
			if (m_State != TB_IDLE && !m_HasTarget)
			{
				SetState(TB_IDLE);
			}
			else if (m_State == TB_CHASE && m_HasTarget)
			{
				// 追跡を外れたら調査移動に戻す
				SetState(TB_SUSPICION);
			}
		}
	}
}

// -------------------------------------------------------
// 直線移動（壁判定なし・経路探索なし）
// -------------------------------------------------------
void TutorialBusters::MoveTo(const XMFLOAT3& targetPos)
{
	float dx = targetPos.x - m_Position.x;
	float dz = targetPos.z - m_Position.z;
	float len = sqrtf(dx * dx + dz * dz);

	if (len < m_MoveSpeed) return;

	dx /= len;
	dz /= len;

	// 向き更新
	float deg = XMConvertToDegrees(atan2f(dx, dz));
	SetRotY(deg + 180.0f);

	m_Position.x += dx * m_MoveSpeed;
	m_Position.z += dz * m_MoveSpeed;
}

// -------------------------------------------------------
// ヘッドライト位置・向き更新
// -------------------------------------------------------
void TutorialBusters::UpdateHeadlight(void)
{
	if (!m_pHeadlight) return;

	XMFLOAT3 headPos = m_Position;
	headPos.y += 2.0f;

	float rotRad = XMConvertToRadians(GetRot().y);
	float dirX = sinf(rotRad);
	float dirZ = cosf(rotRad);
	headPos.x += dirX * 0.8f;
	headPos.z += dirZ * 0.8f;

	m_pHeadlight->SetPosition(headPos.x, headPos.y, headPos.z);
	m_pHeadlight->SetDirection(XMFLOAT4(dirX, -0.3f, dirZ, 0.0f));

	float range = 15.0f;
	switch (m_State)
	{
	case TB_SUSPICION: range = BUSTERS_SUSPICION_RANGE; break;
	case TB_CHASE:     range = BUSTERS_PATROL_RANGH;    break;
	case TB_STUN:      range = 5.0f;                    break;
	default:           range = 15.0f;                   break;
	}
	m_pHeadlight->SetRange(range);
}

// -------------------------------------------------------
// Update
// -------------------------------------------------------
void TutorialBusters::Update(void)
{
	const float dt = 1.0f / 60.0f;
	this->UpdateAnimation(dt);

	// 状態に応じたモーション切り替え（本元Bustersと同様）
	if (m_IsExiting)
	{
		this->PlayAnimationByName("walk", true);
	}
	else
	{
		switch (m_State)
		{
		case TB_IDLE:
			this->PlayAnimationByName("walk", true);
			break;
		case TB_SUSPICION:
			if (m_WaitTimer > 0 && m_HasTarget)
				this->PlayAnimationByName("chousa", true);
			else
				this->PlayAnimationByName("walk", true);
			break;
		case TB_CHASE:
			// hakkenが再生中なら終了を待ち、終わったらhakkendashへ
			if (m_AnimState.currentAnimName == "hakken")
			{
				if (!this->IsAnimationPlaying())
				{
					this->PlayAnimationByName("hakkendash", true);
					m_WaitTimer = 0;
				}
			}
			else if (m_AnimState.currentAnimName != "hakkendash")
			{
				// CHASE状態に入った直後：まずhakkenを非ループで再生
				this->PlayAnimationByName("hakken", false);
			}
			break;
		case TB_STUN:
			this->PlayAnimationByName("kizetsu", true);
			break;
		}
	}

	JumpUpdate(*(Transform3D*)this);

	// 退場中は退場先へ歩くだけ
	if (m_IsExiting)
	{
		float dx = m_ExitTargetPos.x - m_Position.x;
		float dz = m_ExitTargetPos.z - m_Position.z;
		float distSq = dx * dx + dz * dz;

		if (distSq <= m_MoveSpeed * m_MoveSpeed)
		{
			// 到着 → 非表示にする
			m_IsExiting = false;
			TutorialObject_SetBustersVisible(false);
		}
		else
		{
			MoveTo(m_ExitTargetPos);
		}

		UpdateHeadlight();

		if (m_Icon)
		{
			m_Icon->SetIcon(BILLBOARD_ICON::NONE);
			XMFLOAT3 iconPos = m_Position;
			iconPos.y += 3.25f;
			m_Icon->SetPos(iconPos);
			m_Icon->Update();
		}
		return;
	}

	// 硬直タイマー
	if (m_WaitTimer > 0)
	{
		m_WaitTimer--;
		Ghost* ghost = GetGhost();
		if (m_HasTarget && m_State == TB_SUSPICION)
		{
			// 調査中はピアノの方向を向き続ける
			float dx = m_TargetPos.x - m_Position.x;
			float dz = m_TargetPos.z - m_Position.z;
			if (dx * dx + dz * dz > 0.001f)
				SetRotY(XMConvertToDegrees(atan2f(dx, dz)) + 180.0f);
		}
		else if (ghost && (m_State == TB_SUSPICION || m_State == TB_CHASE))
		{
			float dx = ghost->GetPos().x - m_Position.x;
			float dz = ghost->GetPos().z - m_Position.z;
			SetRotY(XMConvertToDegrees(atan2f(dx, dz)) + 180.0f);
		}
	}
	else
	{
		// 状態チェック（ゴースト検知）
		CheckState();

		Ghost* ghost = GetGhost();

		if (m_State == TB_CHASE || m_State == TB_SUSPICION)
		{
			if (m_HasTarget && m_State == TB_SUSPICION)
			{
				// 調査対象（ピアノ）へ向かっているとき
				float dx = m_TargetPos.x - m_Position.x;
				float dz = m_TargetPos.z - m_Position.z;
				float distSq = dx * dx + dz * dz;

				if (distSq <= 2.0f * 2.0f)
				{
					// ピアノ前に到着 → 調査開始（立ち止まって CHECK アイコン）
					m_WaitTimer = 180;
					if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::CHECK);
					// ターゲット方向を向く
					if (distSq > 0.001f)
					{
						SetRotY(XMConvertToDegrees(atan2f(dx, dz)) + 180.0f);
					}
				}
				else
				{
					// まだ遠い → 近づく
					m_MoveSpeed = BUSTERS_MOVE_SPEED_SUSPICION;
					MoveTo(m_TargetPos);
				}
			}
			else if (ghost)
			{
				// ゴーストを発見している → ゴーストへ直線移動
				m_MoveSpeed = (m_State == TB_CHASE)
					? BUSTERS_MOVE_SPEED_CHASE
					: BUSTERS_MOVE_SPEED_SUSPICION;
				MoveTo(ghost->GetPos());
			}
		}
		else if (m_HasTarget && m_State == TB_IDLE)
		{
			// 調査対象（ピアノ）へ警戒速度で向かう
			m_MoveSpeed = BUSTERS_MOVE_SPEED_SUSPICION;
			SetState(TB_SUSPICION);
			MoveTo(m_TargetPos);
		}
	}

	// ヘッドライト更新
	UpdateHeadlight();

	// アイコン更新
	if (m_Icon)
	{
		// 調査中（ピアノ前で待機中）は CHECK アイコンをキープする
		bool keepCheck = (m_State == TB_SUSPICION && m_WaitTimer > 0 && m_HasTarget);
		if (!keepCheck)
		{
			switch (m_State)
			{
			case TB_IDLE:      m_Icon->SetIcon(BILLBOARD_ICON::NONE);     break;
			case TB_SUSPICION: m_Icon->SetIcon(BILLBOARD_ICON::QUESTION); break;
			case TB_CHASE:     m_Icon->SetIcon(BILLBOARD_ICON::ALERT);    break;
			case TB_STUN:      m_Icon->SetIcon(BILLBOARD_ICON::STUN);     break;
			}
		}

		XMFLOAT3 iconPos = m_Position;
		iconPos.y += 3.25f;
		m_Icon->SetPos(iconPos);
		m_Icon->Update();
	}
}

void TutorialBusters::Draw(void)
{
	AnimSprite3D::Draw();

	if (m_Icon)
	{
		Shader_Begin();
		m_Icon->Draw();
	}
}

void TutorialBusters::SetState(TUTORIAL_BUSTERS_STATE state)
{
	m_State = state;

	switch (m_State)
	{
	case TB_IDLE:
		this->ResetColor();
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::NONE);
		break;

	case TB_SUSPICION:
		this->SetColor(1.0f, 1.0f, 0.0f, 1.0f); // 黄
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::QUESTION);
		break;

	case TB_CHASE:
		this->SetColor(1.0f, 0.0f, 0.0f, 1.0f); // 赤
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::ALERT);
		break;

	case TB_STUN:
		this->SetColor(0.0f, 0.0f, 1.0f, 1.0f); // 青
		if (m_Icon) m_Icon->SetIcon(BILLBOARD_ICON::STUN);
		break;
	}
}

// 驚かせられた
void TutorialBusters::OnScared(void)
{
	JumpStart();
	SetState(TB_STUN);
	m_WaitTimer      = 180; // 3秒間スタン
	m_KeepStateTimer = 0;
}

// 退場開始
void TutorialBusters::StartExit(const XMFLOAT3& exitPos)
{
	m_IsExiting = true;
	m_ExitTargetPos = exitPos;
	m_HasTarget = false;
	m_WaitTimer = 0;
	m_KeepStateTimer = 0;
	m_MoveSpeed = BUSTERS_MOVE_SPEED_SEARCH;
	SetState(TB_IDLE);
}

// =================================================================
// グローバル関数
// =================================================================

void TutorialObject_Initialize(void)
{
	g_pEnban = new Sprite3D(
		{ -4.5f, 0.0f, 17.0f },
		{ 8.0f, 13.0f, 8.0f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\enban.fbx"
	);

	if (g_pEnban)
	{
		g_pEnban->SetColor(0.0f, 1.0f, 0.0f, 0.5f);
	}

	g_EnbanTouched   = false;
	g_PianoPossessed = false;
	g_BustersStunned = false; // 追加：バスターズスタンフラグのリセット
	g_PossessionOnlyPiano = false;
	g_ScareEnabled = true;
	g_ScareRequireBusterInRange = false;

	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}

	g_pTutorialBusters = new TutorialBusters(
		{ 0.0f, BUSTERS_HEIGHT, 0.0f },
		{ 0.12f, 0.12f, 0.12f },
		{ 0.0f, 180.0f, 0.0f },
		"asset\\model\\busters_v3.fbx"
	);

	if (g_pTutorialMarker)
	{
		delete g_pTutorialMarker;
		g_pTutorialMarker = nullptr;
	}
	g_pTutorialMarker = new TutorialMarker();
	g_pTutorialMarker->Initialize({ -5.0f, 0.5f, 17.0f });
}

void TutorialObject_Update(void)
{
	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->Update();
	}

	if (g_pTutorialMarker)
	{
		g_pTutorialMarker->Update();
	}

	if (!UI_Tutorial_IsWaiting()) return;

	Ghost* pGhost = GetGhost();
	if (!pGhost) return;

	if (g_pEnban && !g_EnbanTouched)
	{
		XMFLOAT3 gPos = pGhost->GetPos();
		XMFLOAT3 ePos = g_pEnban->GetPos();
		float dx = gPos.x - ePos.x;
		float dz = gPos.z - ePos.z;

		float enbanRadius = g_pEnban->GetScale().x * 0.5f;
		if (sqrtf(dx * dx + dz * dz) <= enbanRadius)
		{
			g_EnbanTouched = true;
		}
	}

	if (!g_PianoPossessed)
	{
		if (pGhost->GetState() == GS_TRANSFORM)
		{
			int inRangeNum = pGhost->GetInRangeNum();
			Furniture* pFurniture = GetFurniture(inRangeNum);
			if (pFurniture && pFurniture->GetBlockID() == 62)
			{
				g_PianoPossessed = true;
			}
		}
	}

	// バスターズがスタンしたかチェック
	if (!g_BustersStunned && g_pTutorialBusters)
	{
		if (g_pTutorialBusters->GetState() == TB_STUN)
		{
			g_BustersStunned = true;
		}
	}
}

void TutorialObject_Draw(void)
{
	// マーカーは全フロアで描画（階段誘導Billboardを常時機能させる）
	if (g_pTutorialMarker)
	{
		g_pTutorialMarker->Draw();
	}

	// 3階専用オブジェクトは従来どおり3階のみ描画
	if (Field_GetCurrentFloor() != 2) return;

	if (g_pTutorialBusters && g_BustersVisible)
	{
		g_pTutorialBusters->Draw();
	}

	if (g_pEnban && g_EnbanVisible)
	{
		g_pEnban->Draw();
	}
}

void TutorialObject_Draw2D(void)
{
	if (g_pTutorialMarker)
	{
		g_pTutorialMarker->Draw2D();
	}
}

void TutorialObject_Finalize(void)
{
	delete g_pEnban;
	g_pEnban         = nullptr;
	g_EnbanTouched   = false;
	g_EnbanVisible   = false;
	g_BustersVisible = false;
	g_PianoPossessed = false;
	g_PossessionOnlyPiano = false;
	g_ScareEnabled = true;
	g_ScareRequireBusterInRange = false;

	if (g_pTutorialBusters)
	{
		delete g_pTutorialBusters;
		g_pTutorialBusters = nullptr;
	}

	if (g_pTutorialMarker)
	{
		delete g_pTutorialMarker;
		g_pTutorialMarker = nullptr;
	}
}

bool* TutorialObject_GetEnbanTouchedPtr(void)
{
	return &g_EnbanTouched;
}

void TutorialObject_SetEnbanVisible(bool visible)
{
	g_EnbanVisible = visible;
}

void TutorialObject_SetBustersVisible(bool visible)
{
	g_BustersVisible = visible;
}

void TutorialObject_SetPossessionOnlyPiano(bool onlyPiano)
{
	g_PossessionOnlyPiano = onlyPiano;
}

void TutorialObject_SetScareEnabled(bool enabled)
{
	g_ScareEnabled = enabled;
}

void TutorialObject_SetScareRequireBusterInRange(bool enabled)
{
	g_ScareRequireBusterInRange = enabled;
}

bool TutorialObject_CanPossessFurnitureBlock(int blockID)
{
	if (!g_PossessionOnlyPiano)
	{
		return true;
	}
	return (blockID == 62);
}

bool TutorialObject_IsScareEnabled(void)
{
	return g_ScareEnabled;
}

bool TutorialObject_IsScareRequireBusterInRange(void)
{
	return g_ScareRequireBusterInRange;
}

bool* TutorialObject_GetPianoPossessedPtr(void)
{
	return &g_PianoPossessed;
}

bool* TutorialObject_GetBustersStunnedPtr(void)
{
	return &g_BustersStunned;
}

FlagWithDelay TutorialObject_GetBustersStunnedPtr(int delayFrames)
{
	return { &g_BustersStunned, delayFrames };
}

TutorialBusters* GetTutorialBusters(void)
{
	return g_pTutorialBusters;
}

TutorialMarker* GetTutorialMarker(void)
{
	return g_pTutorialMarker;
}

void TutorialObject_StartBusterExit(const XMFLOAT3& exitPos)
{
	if (g_pTutorialBusters)
	{
		g_pTutorialBusters->StartExit(exitPos);
	}
}

void TutorialBusters_SetLight(void)
{
	if (!g_pTutorialBusters || !g_BustersVisible) return;

	PointLight* pLight = g_pTutorialBusters->GetHeadlight();
	if (pLight)
	{
		Shader_AddPointLight(pLight);
	}
}
