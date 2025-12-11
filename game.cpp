/*==============================================================================

   ポリゴン描画 [game.cpp]

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "main.h"
#include "shader.h"
#include "debug_ostream.h"
#include "game.h"
#include "field.h"
#include "texture.h"
#include "keyboard.h"
#include "scene.h"
#include "camera.h"
#include "sprite.h"
#include "UI.h"
#include "ghost.h"
#include "furniture.h"
#include "busters.h"
#include "debugdraw.h"

Light* MainLight;

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Game_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	MainLight = new Light
	(TRUE,
		XMFLOAT4(0.0f, -10.0f, -10.0f, 1.0f),	//向き
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//光の色
		XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)	//環境光
	);

	Camera_Initialize();
	Ghost_Initialize(pDevice, pContext);
	Field_Initialize(pDevice, pContext);
	UI_Initialize();
	Furniture_Initialize();
	Busters_Initialize();
	DebugDraw_Initialize();
}

void Game_Update(void)
{
	Ghost_Update();
	Camera_Update();
	Field_Update();
	UI_Update();
	Furniture_Update();
	Busters_Update();
	DebugDraw_Update();
}

void Game_Draw(void)
{
	MainLight->SetEnable(true);
	Shader_SetLight(MainLight);

	//3D描画の前に深度テストを有効にする
	SetDepthTest(true);

	Field_Draw();
	Ghost_Draw();
	Furniture_Draw();
	Busters_Draw();
	DebugDraw_Draw();

	SetDepthTest(false);
	MainLight->SetEnable(false);
	Shader_SetLight(MainLight);

	//2D描画処理をここに記述
	UI_Draw();
}

void Game_Finalize(void)
{
	delete MainLight;

	Camera_Finalize();
	Ghost_Finalize();
	Field_Finalize();
	UI_Finalize();
	Furniture_Finalize();
	Busters_Finalize();
	DebugDraw_Finalize();
}