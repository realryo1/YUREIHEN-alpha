#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "ghost.h"
#include "sprite3d.h"
#include "component.h"
#include "define.h"

using namespace DirectX;

// Furniture クラス - 色を変更でき、ジャンプ機能を持つ
class Furniture : public Sprite3D, public Jump
{
protected:
	//ghostとの距離を保持する変数
	float m_DistanceToGhost;
public:
	Furniture(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass)
		: Sprite3D(pos, scale, rot, pass),
		  Jump(0.01f, 0.2f, 1.0f), m_DistanceToGhost()  // gravity, jumpPower, groundLevel
	{
	}

	~Furniture() = default;

	// 更新メソッド（ジャンプ処理とghostとの距離）
	void Update(void)
	{
		// ジャンプ状態の更新（基クラスJumpのJumpUpdateを呼ぶ）
		// Furniture は Sprite3D でもあり Jump でもあるため、
		// Sprite3D（Transform3D）への参照として渡す
		JumpUpdate(*(Transform3D*)this);

		// ghostとの距離計算
		// Ghost と Furniture の距離を計算
		XMFLOAT3 furniturePos = m_Position;
		XMFLOAT3 ghostPos = GetGhost()->GetPos();
		XMVECTOR ghostVec = XMLoadFloat3(&ghostPos);
		XMVECTOR furnitureVec = XMLoadFloat3(&furniturePos);
		XMVECTOR distVec = XMVectorSubtract(furnitureVec, ghostVec);
		m_DistanceToGhost =  XMVectorGetX(XMVector3Length(distVec));
	}

	//ゲッター	
	float GetDistanceToGhost(void) const
	{
		return m_DistanceToGhost;
	}
};

void Furniture_Initialize(void);
void Furniture_Update(void);
void Furniture_Draw(void);
void Furniture_Finalize(void);

// ゲッター関数
Furniture* GetFurniture(int index);

// ジャンプ関数
void FurnitureScare(int index);
