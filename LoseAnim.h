#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

void Animation_Lose_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Animation_Lose_Update(void);
void Animation_Lose_Draw(void);
void Animation_Lose_Finalize(void);

