#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "sprite.h"
#include "ClickFont.h"
using namespace DirectX;

// ==========================================
// ポーズメニュー
// ==========================================

void UI_PauseMenu_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void UI_PauseMenu_Finalize(void);
void UI_PauseMenu_Update(void);
void UI_PauseMenu_Draw(void);

bool UI_PauseMenu_IsPaused(void);
void UI_PauseMenu_SetPause(bool isPause);
float UI_PauseMenu_GetBrightness(void);
void UI_PauseMenu_HideSkipFloorButton(void);
