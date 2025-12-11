#pragma once

#include <directxmath.h>
using namespace DirectX;


class Transform3D
{
protected:
	XMFLOAT3 m_Position;
	XMFLOAT3 m_Rotation;
	XMFLOAT3 m_Scale;
public:
	Transform3D(XMFLOAT3 p = XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3 r = XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3 s = XMFLOAT3(0.0f, 0.0f, 0.0f)) :
		m_Position(p), m_Rotation(r), m_Scale(s)
	{
	}
	XMFLOAT3 GetPos(void) const {
		return m_Position;
	}
	XMFLOAT3 GetRot(void) const {
		return m_Rotation;
	}
	XMFLOAT3 GetScale(void) const {
		return m_Scale;
	}
	void SetPos(XMFLOAT3 p) {
		m_Position = p;
	}
	void SetRot(XMFLOAT3 r) {
		m_Rotation = r;
	}
	void SetSize(XMFLOAT3 s) {
		m_Scale = s;
	}
	XMFLOAT3 AddPos(XMFLOAT3 p) {
		m_Position.x += p.x;
		m_Position.y += p.y;
		m_Position.z += p.z;
		return m_Position;
	}
	XMFLOAT3 AddRot(XMFLOAT3 r) {
		m_Rotation.x += r.x;
		m_Rotation.y += r.y;
		m_Rotation.z += r.z;
		return m_Rotation;
	}
	XMFLOAT3 AddScale(XMFLOAT3 s) {
		m_Scale.x += s.x;
		m_Scale.y += s.y;
		m_Scale.z += s.z;
		return m_Scale;
	}

	// Position X,Y,Z個別ゲッター/セッター/アッダー
	float GetPosX(void) const { return m_Position.x; }
	float GetPosY(void) const { return m_Position.y; }
	float GetPosZ(void) const { return m_Position.z; }
	void SetPosX(float x) { m_Position.x = x; }
	void SetPosY(float y) { m_Position.y = y; }
	void SetPosZ(float z) { m_Position.z = z; }
	float AddPosX(float x) { m_Position.x += x; return m_Position.x; }
	float AddPosY(float y) { m_Position.y += y; return m_Position.y; }
	float AddPosZ(float z) { m_Position.z += z; return m_Position.z; }

	// Rotation X,Y,Z個別ゲッター/セッター/アッダー
	float GetRotX(void) const { return m_Rotation.x; }
	float GetRotY(void) const { return m_Rotation.y; }
	float GetRotZ(void) const { return m_Rotation.z; }
	void SetRotX(float x) { m_Rotation.x = x; }
	void SetRotY(float y) { m_Rotation.y = y; }
	void SetRotZ(float z) { m_Rotation.z = z; }
	float AddRotX(float x) { m_Rotation.x += x; return m_Rotation.x; }
	float AddRotY(float y) { m_Rotation.y += y; return m_Rotation.y; }
	float AddRotZ(float z) { m_Rotation.z += z; return m_Rotation.z; }

	// Scale X,Y,Z個別ゲッター/セッター/アッダー
	float GetScaleX(void) const { return m_Scale.x; }
	float GetScaleY(void) const { return m_Scale.y; }
	float GetScaleZ(void) const { return m_Scale.z; }
	void SetScaleX(float x) { m_Scale.x = x; }
	void SetScaleY(float y) { m_Scale.y = y; }
	void SetScaleZ(float z) { m_Scale.z = z; }
	float AddScaleX(float x) { m_Scale.x += x; return m_Scale.x; }
	float AddScaleY(float y) { m_Scale.y += y; return m_Scale.y; }
	float AddScaleZ(float z) { m_Scale.z += z; return m_Scale.z; }
};

// 2D 用の Transform クラス
class Transform2D
{
protected:
	XMFLOAT2 m_Position;
	float    m_Rotation; // degrees
	XMFLOAT2 m_Scale;
public:
	Transform2D(XMFLOAT2 p = XMFLOAT2(0.0f, 0.0f), float r = 0.0f, XMFLOAT2 s = XMFLOAT2(1.0f, 1.0f)) :
		m_Position(p), m_Rotation(r), m_Scale(s)
	{
	}

	// getters
	XMFLOAT2 GetPos() const { return m_Position; }
	float GetRot() const { return m_Rotation; }
	XMFLOAT2 GetScale() const { return m_Scale; }

	// setters
	void SetPos(const XMFLOAT2& p) { m_Position = p; }
	void SetRot(float r) { m_Rotation = r; }
	void SetSize(const XMFLOAT2& s) { m_Scale = s; }

	// adders
	XMFLOAT2 AddPos(const XMFLOAT2& p) { m_Position.x += p.x; m_Position.y += p.y; return m_Position; }
	float AddRot(float r) { m_Rotation += r; return m_Rotation; }
	XMFLOAT2 AddScale(const XMFLOAT2& s) { m_Scale.x += s.x; m_Scale.y += s.y; return m_Scale; }

	// Position X,Y個別ゲッター/セッター/アッダー
	float GetPosX(void) const { return m_Position.x; }
	float GetPosY(void) const { return m_Position.y; }
	void SetPosX(float x) { m_Position.x = x; }
	void SetPosY(float y) { m_Position.y = y; }
	float AddPosX(float x) { m_Position.x += x; return m_Position.x; }
	float AddPosY(float y) { m_Position.y += y; return m_Position.y; }

	// Scale X,Y個別ゲッター/セッター/アッダー
	float GetScaleX(void) const { return m_Scale.x; }
	float GetScaleY(void) const { return m_Scale.y; }
	void SetScaleX(float x) { m_Scale.x = x; }
	void SetScaleY(float y) { m_Scale.y = y; }
	float AddScaleX(float x) { m_Scale.x += x; return m_Scale.x; }
	float AddScaleY(float y) { m_Scale.y += y; return m_Scale.y; }
};

// 衝突判定関連
enum class CollisionFace
{
    NONE = 0,
    LEFT,      // -X面
    RIGHT,     // +X面
    BOTTOM,    // -Y面
    TOP,       // +Y面
    BACK,      // -Z面
    FRONT      // +Z面
};

struct CollisionResult
{
    bool isColliding;
    CollisionFace face;
    float penetrationDepth;

    CollisionResult() : isColliding(false), face(CollisionFace::NONE), penetrationDepth(0.0f) {}
};

// 当たり判定クラス
class BoxCollider
{
protected:
    XMFLOAT3 m_Size;
    XMFLOAT3 m_Offset;
    bool m_Active;
    bool m_IsTrigger;
    bool m_DebugDraw;

public:
    BoxCollider(XMFLOAT3 size, bool isTrigger = false, XMFLOAT3 offset = XMFLOAT3(0.0f, 0.0f, 0.0f),
         bool active = true, bool debugDraw = false) :
        m_Size(size), m_Offset(offset), m_IsTrigger(isTrigger),
        m_Active(active), m_DebugDraw(debugDraw)
    {
    }

    // 衝突判定と位置補正
    CollisionResult CheckCollisionAndResolve(Transform3D& myTransform,
        const BoxCollider& other,
        const Transform3D& otherTransform)
    {
        CollisionResult result;

        if (!m_Active || !other.m_Active) {
            return result;
        }

        // 各BoxColliderの中心位置を計算
        XMFLOAT3 myCenter = XMFLOAT3(
            myTransform.GetPos().x + m_Offset.x,
            myTransform.GetPos().y + m_Offset.y,
            myTransform.GetPos().z + m_Offset.z
        );

        XMFLOAT3 otherCenter = XMFLOAT3(
            otherTransform.GetPos().x + other.m_Offset.x,
            otherTransform.GetPos().y + other.m_Offset.y,
            otherTransform.GetPos().z + other.m_Offset.z
        );

        // 各軸の距離を計算
        float dx = otherCenter.x - myCenter.x;
        float dy = otherCenter.y - myCenter.y;
        float dz = otherCenter.z - myCenter.z;

        // 各軸のハーフサイズの合計
        float halfWidthSum = (m_Size.x + other.m_Size.x) / 2.0f;
        float halfHeightSum = (m_Size.y + other.m_Size.y) / 2.0f;
        float halfDepthSum = (m_Size.z + other.m_Size.z) / 2.0f;

        // 各軸での侵入深度を計算
        float overlapX = halfWidthSum - fabsf(dx);
        float overlapY = halfHeightSum - fabsf(dy);
        float overlapZ = halfDepthSum - fabsf(dz);

        // すべての軸で重なりがあるか確認
        if (overlapX > 0 && overlapY > 0 && overlapZ > 0)
        {
            result.isColliding = true;

            // 最小の重なり（侵入深度）を持つ軸が衝突面を決定
            float minOverlap = overlapX;
            CollisionFace face = (dx > 0) ? CollisionFace::RIGHT : CollisionFace::LEFT;
            int axis = 0; // 0=X, 1=Y, 2=Z

            if (overlapY < minOverlap)
            {
                minOverlap = overlapY;
                face = (dy > 0) ? CollisionFace::TOP : CollisionFace::BOTTOM;
                axis = 1;
            }

            if (overlapZ < minOverlap)
            {
                minOverlap = overlapZ;
                face = (dz > 0) ? CollisionFace::FRONT : CollisionFace::BACK;
                axis = 2;
            }

            result.face = face;
            result.penetrationDepth = minOverlap;

            // IsTriggerがfalseなら位置を補正して貫通を防ぐ
            if (!m_IsTrigger && !other.m_IsTrigger)
            {
                XMFLOAT3 currentPos = myTransform.GetPos();

                switch (axis)
                {
                case 0: // X軸で衝突
                    if (dx > 0) {
                        currentPos.x -= minOverlap;
                    }
                    else {
                        currentPos.x += minOverlap;
                    }
                    break;

                case 1: // Y軸で衝突
                    if (dy > 0) {
                        currentPos.y -= minOverlap;
                    }
                    else {
                        currentPos.y += minOverlap;
                    }
                    break;

                case 2: // Z軸で衝突
                    if (dz > 0) {
                        currentPos.z -= minOverlap;
                    }
                    else {
                        currentPos.z += minOverlap;
                    }
                    break;
                }

                myTransform.SetPos(currentPos);
            }
        }

        return result;
    }

    // 読み取り専用版（constバージョン）
    CollisionResult CheckCollisionDetailed(const Transform3D& myTransform,
        const BoxCollider& other,
        const Transform3D& otherTransform) const
    {
        CollisionResult result;

        if (!m_Active || !other.m_Active) {
            return result;
        }

        XMFLOAT3 myCenter = XMFLOAT3(
            myTransform.GetPos().x + m_Offset.x,
            myTransform.GetPos().y + m_Offset.y,
            myTransform.GetPos().z + m_Offset.z
        );

        XMFLOAT3 otherCenter = XMFLOAT3(
            otherTransform.GetPos().x + other.m_Offset.x,
            otherTransform.GetPos().y + other.m_Offset.y,
            otherTransform.GetPos().z + other.m_Offset.z
        );

        float dx = otherCenter.x - myCenter.x;
        float dy = otherCenter.y - myCenter.y;
        float dz = otherCenter.z - myCenter.z;

        float halfWidthSum = (m_Size.x + other.m_Size.x) / 2.0f;
        float halfHeightSum = (m_Size.y + other.m_Size.y) / 2.0f;
        float halfDepthSum = (m_Size.z + other.m_Size.z) / 2.0f;

        float overlapX = halfWidthSum - fabsf(dx);
        float overlapY = halfHeightSum - fabsf(dy);
        float overlapZ = halfDepthSum - fabsf(dz);

        if (overlapX > 0 && overlapY > 0 && overlapZ > 0)
        {
            result.isColliding = true;

            float minOverlap = overlapX;
            CollisionFace face = (dx > 0) ? CollisionFace::RIGHT : CollisionFace::LEFT;

            if (overlapY < minOverlap)
            {
                minOverlap = overlapY;
                face = (dy > 0) ? CollisionFace::TOP : CollisionFace::BOTTOM;
            }

            if (overlapZ < minOverlap)
            {
                minOverlap = overlapZ;
                face = (dz > 0) ? CollisionFace::FRONT : CollisionFace::BACK;
            }

            result.face = face;
            result.penetrationDepth = minOverlap;
        }

        return result;
    }

	XMFLOAT3 GetSize(void) const { return m_Size; }
	XMFLOAT3 GetOffset(void) const { return m_Offset; }
	bool GetIsTrigger(void) const { return m_IsTrigger; }
	bool GetActive(void) const { return m_Active; }
	bool GetDebugDraw(void) const { return m_DebugDraw; }
	void SetSize(XMFLOAT3 size) { m_Size = size; }
	void SetOffset(XMFLOAT3 offset) { m_Offset = offset; }
	void SetIsTrigger(bool isTrigger) { m_IsTrigger = isTrigger; }
	void SetActive(bool active) { m_Active = active; }
	void SetDebugDraw(bool debugDraw) { m_DebugDraw = debugDraw; }
};

// Jumpコンポーネント - ジャンプ機能を提供する
class Jump
{
protected:
	float m_JumpVelocityY;      // Y方向の速度
	float m_Gravity;            // 重力加速度
	float m_JumpPower;          // ジャンプ力
	float m_GroundLevel;        // 地面の高さ
	bool m_IsJumping;           // ジャンプ中フラグ
	bool m_IsJumpEnded;         // ジャンプ終了フラグ

public:
	Jump(float gravity = 0.01f, float jumpPower = 0.2f, float groundLevel = 1.0f)
		: m_IsJumping(false), m_JumpVelocityY(0.0f), m_Gravity(gravity), 
		m_JumpPower(jumpPower), m_GroundLevel(groundLevel), m_IsJumpEnded(true)
	{
	}

	~Jump() = default;

	// ジャンプを開始する
	void JumpStart(void)
	{
		if (!m_IsJumping)
		{
			m_IsJumpEnded = false;
			m_IsJumping = true;
			m_JumpVelocityY = m_JumpPower;
		}
	}

	// ジャンプ状態を更新（Transform3D を受け取る）
	void JumpUpdate(Transform3D& transform)
	{
		if (m_IsJumping)
		{
			// Y位置を更新
			XMFLOAT3 pos = transform.GetPos();
			pos.y += m_JumpVelocityY;
			transform.SetPos(pos);

			// 速度に重力を適用
			m_JumpVelocityY -= m_Gravity;

			// 地面に着地したかチェック
			if (pos.y <= m_GroundLevel)
			{
				pos.y = m_GroundLevel;
				transform.SetPos(pos);
				m_IsJumping = false;
				m_IsJumpEnded = true;
				m_JumpVelocityY = 0.0f;
			}
		}
	}

	// ジャンプ中かどうかを取得
	bool GetIsJumping(void) const
	{
		return m_IsJumping;
	}

	// ジャンプ終了したかどうかを取得
	bool GetIsJumpEnded(void) const
	{
		return m_IsJumpEnded;
	}

	// ジャンプ速度を取得
	float GetJumpVelocityY(void) const
	{
		return m_JumpVelocityY;
	}

	// 地面の高さを設定
	void SetGroundLevel(float groundLevel)
	{
		m_GroundLevel = groundLevel;
	}

	// 地面の高さを取得
	float GetGroundLevel(void) const
	{
		return m_GroundLevel;
	}
};
