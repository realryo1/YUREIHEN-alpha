#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// 各アニメーションシーン向けのライフサイクル関数
void Animation_Logo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Animation_Logo_Update(void);
void Animation_Logo_Draw(void);
void Animation_Logo_Finalize(void);

void Animation_Op_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Animation_Op_Update(void);
void Animation_Op_Draw(void);
void Animation_Op_Finalize(void);
