#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "model.h"
#include "anim_sprite3d.h"

using namespace DirectX;

void DebugDraw_Initialize(void);
void DebugDraw_Update(void);
void DebugDraw_Draw(void);
void DebugDraw_Finalize(void);
