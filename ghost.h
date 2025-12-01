#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "sprite3d.h"
#include "define.h"

using namespace DirectX;

// Ghost クラス
class Ghost : public Sprite3D
{
private:
	XMFLOAT3 m_Velocity;		// Ghost の速度ベクトル
	int m_InRangeFurnitureNum;	// 範囲内にいる家具の番号（いないなら-1）
	bool m_IsTransformed;		// 変身しているか
	bool m_IsDetectedByBuster;	// bustarに発見されたか
	float m_DetectionTimer;		// 発見状態のタイマー（1秒につきマイナス1するため）

public:
	Ghost(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Sprite3D(pos, scale, rot, pass),
		m_Velocity(0.0f, 0.0f, 0.0f),
		m_InRangeFurnitureNum(-1),
		m_IsTransformed(false),
		m_IsDetectedByBuster(false),
		m_DetectionTimer(0.0f)
	{
	}

	~Ghost() = default;

	// ゲッター
	XMFLOAT3 GetVelocity(void) const { return m_Velocity; }
	int GetInRangeNum(void) const { return m_InRangeFurnitureNum; }
	bool GetIsTransformed(void) const { return m_IsTransformed; }
	bool GetIsDetectedByBuster(void) const { return m_IsDetectedByBuster; }

	// セッター
	void SetVelocity(const XMFLOAT3& velocity) { m_Velocity = velocity; }
	void SetInRangeNum(int num) { m_InRangeFurnitureNum = num; }
	void SetIsTransformed(bool isTransformed) { m_IsTransformed = isTransformed; }
	void SetIsDetectedByBuster(bool isDetected) { m_IsDetectedByBuster = isDetected; }

	// 公開メソッド
	void UpdateFurnitureDetection(void);  // 家具検知と色変更
	void UpdateInput(void);                // キー入力処理
	void UpdateMovement(void);             // 移動処理
	void ResetState(void);                 // 状態リセット

	// 定数アクセサ
	static float GetDetectionRange(void) { return FURNITURE_DETECTION_RANGE; }
	static float GetGhostPosY(void) { return GHOST_POS_Y; }
};

void Ghost_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Ghost_Update(void);
void Ghost_Draw(void);
void Ghost_Finalize(void);

//ghostのゲッター
Ghost* GetGhost(void);