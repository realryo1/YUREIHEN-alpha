#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"

using namespace DirectX;

void ModelDraw_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ModelDraw_Update(void);
void ModelDraw_DrawAll(void);
void ModelDraw_Finalize(void);
