#pragma once
#include "component.h"

#define BOX_NUM_VERTEX (24)

void CreateBox(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
	ID3D11Buffer** ppVertexBuffer, ID3D11Buffer** ppIndexBuffer);

class Box : public Transform, public BoxCollider
{

public:
	Box(XMFLOAT3 pos, XMFLOAT3 size, bool isTrigger) :
		Transform(pos), BoxCollider(size, isTrigger)
	{
	}
};
