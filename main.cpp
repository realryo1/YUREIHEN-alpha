//==================================================
//main.cpp
//制作者：鶴岡遼大
//制作日：2025/05/09
//==================================================

#include <SDKDDKVer.h> //利用できる最も上位のWindowsプラットフォームが定義される
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を無視
#include <windows.h>
#include <algorithm>
#include "scene.h"
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"
#include "main.h"
#include "keyboard.h"
#include "mouse.h"
#include "sprite.h"
#include "fade.h"

//==================================
//グローバル変数
//==================================

//#ifndef _DEBUG
int g_CountFPS;
char g_DebugStr[2048];
//#endif

#pragma comment(lib, "winmm.lib")

//==================================
//メイン関数
//==================================
//==================================  
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	//フレームレート計測用変数
	DWORD dwExecLastTime;
	DWORD dwFPSLastTime;
	DWORD dwCurrentTime;
	DWORD dwFrameCount;

	HRESULT dummy = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

	//ウィンドウクラスの登録
	WNDCLASS wc;//構造体を定義
	ZeroMemory(&wc, sizeof(WNDCLASS));//構造体初期化
	wc.lpfnWndProc = WndProc;//初期化
	wc.lpszClassName = CLASS_NAME;//仕様書の名前
	wc.hInstance = hInstance;//このアプリのこと
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);//cursorの種類
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);//背景色
	RegisterClass(&wc);//構造体をwindowsにセット

	//ウィンドウサイズの調整
	//クライアント領域（描画領域）のサイズを表す矩形
	RECT window_rect = { 0,0,(LONG)SCREEN_WIDTH,(LONG)SCREEN_HEIGHT };
	//ウィンドウスタイルの設定
	DWORD window_style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
	//指定のクライアント領域＋ウィンドウスタイルでの全体のサイズを計算
	AdjustWindowRect(&window_rect, window_style, FALSE);
	//矩形の横と縦のサイズを計算
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	//ウィンドウの作成
	HWND hWnd = CreateWindow(
		CLASS_NAME,		//作りたいウィンドウ
		WINDOW_CAPTION,	//ウィンドウに表示するタイトル
		window_style,//標準的なサイズのウィンドウ　サイズ変更禁止
		CW_USEDEFAULT,	//以下default
		CW_USEDEFAULT,
		window_width,//ウィンドウの幅
		window_height,//ウィンドウの高さ
		NULL,
		NULL,
		hInstance,	//アプリのハンドル
		NULL
	);

	ShowWindow(hWnd, nCmdShow);//引数に従って表示非表示

	//ウィンドウ内部の更新要求
	UpdateWindow(hWnd);
	Direct3D_Initialize(hWnd);
	Keyboard_Initialize();
	Mouse_Initialize(hWnd);
	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	Sprite_Initialize();
	Fade_Initialize();
	Init();

	//メッセージループ
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	//フレームレート計測初期化
	timeBeginPeriod(1);	//タイマーの制度を設定　
	dwExecLastTime = dwFPSLastTime = timeGetTime();
	dwCurrentTime = dwFrameCount = 0;

	do
	{
		//終了メッセージが来るまでループ （Windowsからのメッセージはそのまま使えない）
		//while (GetMessage(&msg, NULL, 0, 0)) ゲーム向きではないらしい
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))	//余計なことをしないので早い
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg); //WndProcが呼び出される
		}
		//ゲームの処理
		else
		{
			dwCurrentTime = timeGetTime();

			if ((dwCurrentTime - dwFPSLastTime) >= 1000)
			{
				//#ifndef _DEBUG
				g_CountFPS = dwFrameCount;
				//#endif
				dwFPSLastTime = dwCurrentTime;
				dwFrameCount = 0;
			}

			if ((dwCurrentTime - dwExecLastTime) >= ((float)1000 / FPS))
			{
				dwExecLastTime = dwCurrentTime;

				//#ifndef _DEBUG
								//ウィンドウキャプションへ現在のFPSを表示
				wsprintf(g_DebugStr, "DX21");
				wsprintf(&g_DebugStr[strlen(g_DebugStr)], "FPS：%d", g_CountFPS);
				SetWindowText(hWnd, g_DebugStr);
				//#endif
				//更新
				Fade_Update();
				Update();

				//描画
				Direct3D_Clear();//バッファのクリア

				Draw();
				Fade_Draw();

				Direct3D_Present();//バッファの表示

				keycopy();

				dwFrameCount++;
			}
		}

	} while (msg.message != WM_QUIT);//windowsから終了メッセージが来たらループ終了

	Finalize();
	Fade_Finalize();
	Sprite_Finalize();
	Shader_Finalize();
	Direct3D_Finalize();


	//終了
	return (int)msg.wParam;
}

//==================================
//ウィンドウプロシージャ
//メッセージループ内で呼び出し
//==================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//HGDIOBJ hbrWhite, hbrGray;

	//HDC hdc;			//デバイスコンテキスト
	//PAINTSTRUCT ps;		//ウィンドウ画面の大きさなど描画関連の情報

	Mouse_ProcessMessage(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_ACTIVATEAPP:

		break;
	case WM_SYSKEYDOWN:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
	case WM_KEYUP:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
	case WM_SYSKEYUP:
		Keyboard_ProcessMessage(uMsg, wParam, lParam);
		break;
		//case WM_PAINT:	//ウィンドウ表示の命令
		//	hdc = BeginPaint(hWnd, &ps);//描画に関する情報を受け取る
		//	EndPaint(hWnd, &ps);	//表示完了　hdcを開放する
		//	return 0;
		//	break;
	case WM_KEYDOWN:	//キーが押された
		//if (wParam == VK_ESCAPE)
		//{
		//	//ウィンドウを閉じたいリクエストをWindowsに送る
		//	SendMessage(hWnd, WM_CLOSE, 0, 0);
		//}
		Keyboard_ProcessMessage(uMsg, wParam, lParam);

		break;
	case WM_CLOSE:
		hal::dout << "終了確認\n" << std::endl;

		//if (MessageBox(hWnd, "本当に終了してよろしいですか", "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
		if(true)
		{
			//OKが押された
			DestroyWindow(hWnd);
		}
		else
		{
			//終わらない
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	//必要のないメッセージは適当に処理するらしい
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}