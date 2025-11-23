#pragma once
//==================================
//マクロ定義
//==================================
#include <SDKDDKVer.h> //利用できる最も上位のWindowsプラットフォームが定義される
#include <windows.h>
#include <algorithm>
#include "direct3d.h"
#include "scene.h"

#define CLASS_NAME "DX21 Window"
#define WINDOW_CAPTION "ポリゴン描画"
#define SCREEN_WIDTH (1280.0f)
#define SCREEN_HEIGHT (720.0f)
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を無視
#define FPS (60)	

using namespace DirectX;

//==================================
//プロトタイプ宣言
//==================================
//ウィンドウプロシージャ
//コールバック関数は他人が呼び出すもの
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
