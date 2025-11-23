/*==============================================================================

   ポリゴン描画 [game.cpp]

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "main.h"
#include "newShader.h"
#include "debug_ostream.h"
#include "game.h"
#include "field.h"
#include "texture.h"
#include "keyboard.h"
#include "scene.h"
#include "camera.h"
#include "modeldraw.h"
#include "button.h"

Light* MainLight;
static Button* testButton = nullptr;

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Game_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	MainLight = new Light
	(TRUE,
		XMFLOAT4(0.0f, -10.0f, 0.0f, 1.0f), //場所
		XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),	//光の色
		XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)	//環境光
	);

	Camera_Initialize();
	Field_Initialize(pDevice, pContext);
	ModelDraw_Initialize(pDevice, pContext);

	// テストボタン作成（左上の方に配置）
	testButton = new Button(100.0f, 50.0f, 150.0f, 40.0f);
	testButton->SetColor(XMFLOAT4(0.2f, 0.5f, 0.8f, 1.0f));
	testButton->SetHoverColor(XMFLOAT4(0.3f, 0.7f, 1.0f, 1.0f));
}

void Game_Update(void)
{
	Camera_Update();
	Field_Update();
	ModelDraw_Update();

	// ボタン更新
	if (testButton) {
		testButton->Update();
		if (testButton->IsClicked()) {
			hal::dout << "テストボタンがクリックされました" << std::endl;
		}
	}
}

void Game_Draw(void)
{
	MainLight->SetEnable(true);
	Shader_SetLight(MainLight);

	//3D描画なら常に有効にする
	SetDepthTest(true);

	Camera_Draw();
	Field_Draw();
	ModelDraw_DrawAll();

	SetDepthTest(false);
	MainLight->SetEnable(false);
	Shader_SetLight(MainLight);
	//2D描画処理ここから

	// テストボタン描画
	if (testButton) {
		testButton->Draw();
	}
}

void Game_Finalize(void)
{
	if (testButton) {
		delete testButton;
		testButton = nullptr;
	}
	delete MainLight;
	Camera_Finalize();
	Field_Finalize();
	ModelDraw_Finalize();
}
