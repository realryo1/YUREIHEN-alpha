#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

void Animation_Win_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Animation_Win_Update(void);
void Animation_Win_Draw(void);
void Animation_Win_Finalize(void);
