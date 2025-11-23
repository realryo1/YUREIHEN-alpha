//==================================================
//main.cpp
//・ｽ・ｽ・ｽ・ｽﾒ：・ｽﾟ会ｿｽ・ｽﾉ托ｿｽ
//・ｽ・ｽ・ｽ・ｽ・ｽ・ｽF2025/05/09
//==================================================

#include <SDKDDKVer.h> //・ｽ・ｽ・ｽp・ｽﾅゑｿｽ・ｽ・ｽﾅゑｿｽ・ｽ・ｽﾊゑｿｽWindows・ｽv・ｽ・ｽ・ｽb・ｽg・ｽt・ｽH・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ`・ｽ・ｽ・ｽ・ｽ・ｽ
#define WIN32_LEAN_AND_MEAN	//32bit・ｽA・ｽv・ｽ・ｽ・ｽﾉは不・ｽv・ｽﾈ擾ｿｽ・ｽｳ趣ｿｽ
#include <windows.h>
#include <algorithm>
#include "scene.h"
#include "direct3d.h"
#include "newShader.h"

#include "debug_ostream.h"
#include "main.h"
#include "keyboard.h"
#include "mouse.h"

//==================================
//・ｽO・ｽ・ｽ・ｽ[・ｽo・ｽ・ｽ・ｽﾏ撰ｿｽ
//==================================

//#ifndef _DEBUG
int g_CountFPS;
char g_DebugStr[2048];
//#endif

#pragma comment(lib, "winmm.lib")

//==================================
//・ｽ・ｽ・ｽC・ｽ・ｽ・ｽﾖ撰ｿｽ
//==================================
//==================================  
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	//・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ[・ｽg・ｽv・ｽ・ｽ・ｽp・ｽﾏ撰ｿｽ
	DWORD dwExecLastTime;
	DWORD dwFPSLastTime;
	DWORD dwCurrentTime;
	DWORD dwFrameCount;

	HRESULT dummy = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽN・ｽ・ｽ・ｽX・ｽﾌ登・ｽ^
	WNDCLASS wc;//・ｽ\・ｽ・ｽ・ｽﾌゑｿｽ・ｽ`
	ZeroMemory(&wc, sizeof(WNDCLASS));//・ｽ\・ｽ・ｽ・ｽﾌ擾ｿｽ・ｽ・ｽ・ｽ・ｽ
	wc.lpfnWndProc = WndProc;//・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	wc.lpszClassName = CLASS_NAME;//・ｽd・ｽl・ｽ・ｽ・ｽﾌ厄ｿｽ・ｽO
	wc.hInstance = hInstance;//・ｽ・ｽ・ｽﾌア・ｽv・ｽ・ｽ・ｽﾌゑｿｽ・ｽ・ｽ
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);//cursor・ｽﾌ趣ｿｽ・ｽ
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);//・ｽw・ｽi・ｽF
	RegisterClass(&wc);//・ｽ\・ｽ・ｽ・ｽﾌゑｿｽwindows・ｽﾉセ・ｽb・ｽg

	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽT・ｽC・ｽY・ｽﾌ抵ｿｽ・ｽ・ｽ
	//・ｽN・ｽ・ｽ・ｽC・ｽA・ｽ・ｽ・ｽg・ｽﾌ茨ｿｽi・ｽ`・ｽ・ｽﾌ茨ｿｽj・ｽﾌサ・ｽC・ｽY・ｽ・ｽ\・ｽ・ｽ・ｽ・ｽ`
	RECT window_rect = { 0,0,(LONG)SCREEN_WIDTH,(LONG)SCREEN_HEIGHT };
	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽX・ｽ^・ｽC・ｽ・ｽ・ｽﾌ設抵ｿｽ
	DWORD window_style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
	//・ｽw・ｽ・ｽﾌク・ｽ・ｽ・ｽC・ｽA・ｽ・ｽ・ｽg・ｽﾌ茨ｿｽ{・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽX・ｽ^・ｽC・ｽ・ｽ・ｽﾅの全・ｽﾌのサ・ｽC・ｽY・ｽ・ｽ・ｽv・ｽZ
	AdjustWindowRect(&window_rect, window_style, FALSE);
	//・ｽ・ｽ`・ｽﾌ会ｿｽ・ｽﾆ縦・ｽﾌサ・ｽC・ｽY・ｽ・ｽ・ｽv・ｽZ
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽﾌ作成
	HWND hWnd = CreateWindow(
		CLASS_NAME,		//・ｽ・ｽ閧ｽ・ｽ・ｽ・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE
		WINDOW_CAPTION,	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽﾉ表・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ^・ｽC・ｽg・ｽ・ｽ
		window_style,//・ｽW・ｽ・ｽ・ｽI・ｽﾈサ・ｽC・ｽY・ｽﾌウ・ｽB・ｽ・ｽ・ｽh・ｽE・ｽ@・ｽT・ｽC・ｽY・ｽﾏ更・ｽﾖ止
		CW_USEDEFAULT,	//・ｽﾈ会ｿｽdefault
		CW_USEDEFAULT,
		window_width,//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽﾌ包ｿｽ
		window_height,//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽﾌ搾ｿｽ・ｽ・ｽ
		NULL,
		NULL,
		hInstance,	//・ｽA・ｽv・ｽ・ｽ・ｽﾌハ・ｽ・ｽ・ｽh・ｽ・ｽ
		NULL
	);

	ShowWindow(hWnd, nCmdShow);//・ｽ・ｽ・ｽ・ｽ・ｽﾉ従・ｽ・ｽ・ｽﾄ表・ｽ・ｽ・ｽ・ｽ\・ｽ・ｽ

	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽ・ｽ・ｽ・ｽ・ｽﾌ更・ｽV・ｽv・ｽ・ｽ
	UpdateWindow(hWnd);
	Direct3D_Initialize(hWnd);
	Keyboard_Initialize();
	Mouse_Initialize(hWnd);
	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	Init();

	//・ｽ・ｽ・ｽb・ｽZ・ｽ[・ｽW・ｽ・ｽ・ｽ[・ｽv
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	//・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ[・ｽg・ｽv・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	timeBeginPeriod(1);	//・ｽ^・ｽC・ｽ}・ｽ[・ｽﾌ撰ｿｽ・ｽx・ｽ・ｽﾝ抵ｿｽ@
	dwExecLastTime = dwFPSLastTime = timeGetTime();
	dwCurrentTime = dwFrameCount = 0;

	do
	{
		//・ｽI・ｽ・ｽ・ｽ・ｽ・ｽb・ｽZ・ｽ[・ｽW・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾜで・ｿｽ・ｽ[・ｽv ・ｽiWindows・ｽ・ｽ・ｽ・ｽﾌ・ｿｽ・ｽb・ｽZ・ｽ[・ｽW・ｽﾍゑｿｽ・ｽﾌまま使・ｽ・ｽ・ｽﾈゑｿｽ・ｽj
		//while (GetMessage(&msg, NULL, 0, 0)) ・ｽQ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾅはなゑｿｽ・ｽ轤ｵ・ｽ・ｽ
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))	//・ｽ]・ｽv・ｽﾈゑｿｽ・ｽﾆゑｿｽ・ｽ・ｽ・ｽﾈゑｿｽ・ｽﾌで托ｿｽ・ｽ・ｽ
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg); //WndProc・ｽ・ｽ・ｽﾄび出・ｽ・ｽ・ｽ・ｽ・ｽ
		}
		//・ｽQ・ｽ[・ｽ・ｽ・ｽﾌ擾ｿｽ・ｽ・ｽ
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
				wsprintf(g_DebugStr, "DX21");
				wsprintf(&g_DebugStr[strlen(g_DebugStr)], "FPS: %d", g_CountFPS);
				SetWindowText(hWnd, g_DebugStr);
				//#endif
				//・ｽX・ｽV
				Update();

				//・ｽ`・ｽ・ｽ
				Direct3D_Clear();//・ｽo・ｽb・ｽt・ｽ@・ｽﾌク・ｽ・ｽ・ｽA

				Draw();

				Direct3D_Present();//・ｽo・ｽb・ｽt・ｽ@・ｽﾌ表・ｽ・ｽ

				keycopy();

				dwFrameCount++;
			}
		}

	} while (msg.message != WM_QUIT);//windows・ｽ・ｽ・ｽ・ｽI・ｽ・ｽ・ｽ・ｽ・ｽb・ｽZ・ｽ[・ｽW・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ辜具ｿｽ[・ｽv・ｽI・ｽ・ｽ

	Finalize();
	Shader_Finalize();
	Direct3D_Finalize();


	//・ｽI・ｽ・ｽ
	return (int)msg.wParam;
}

//==================================
//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽv・ｽ・ｽ・ｽV・ｽ[・ｽW・ｽ・ｽ
//・ｽ・ｽ・ｽb・ｽZ・ｽ[・ｽW・ｽ・ｽ・ｽ[・ｽv・ｽ・ｽ・ｽﾅ呼び出・ｽ・ｽ
//==================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//HGDIOBJ hbrWhite, hbrGray;

	//HDC hdc;			//・ｽf・ｽo・ｽC・ｽX・ｽR・ｽ・ｽ・ｽe・ｽL・ｽX・ｽg
	//PAINTSTRUCT ps;		//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽ・ｽﾊの大き・ｽ・ｽ・ｽﾈど描・ｽ・ｽﾖ連・ｽﾌ擾ｿｽ・ｽ

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
		//case WM_PAINT:	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽ\・ｽ・ｽ・ｽﾌ厄ｿｽ・ｽ・ｽ
		//	hdc = BeginPaint(hWnd, &ps);//・ｽ`・ｽ・ｽﾉ関ゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽｯ趣ｿｽ・ｽ
		//	EndPaint(hWnd, &ps);	//・ｽ\・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ@hdc・ｽ・ｽ・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
		//	return 0;
		//	break;
	case WM_KEYDOWN:	//・ｽL・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ黷ｽ
		//if (wParam == VK_ESCAPE)
		//{
		//	//・ｽE・ｽB・ｽ・ｽ・ｽh・ｽE・ｽ・ｽﾂゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽN・ｽG・ｽX・ｽg・ｽ・ｽWindows・ｽﾉ托ｿｽ・ｽ・ｽ
		//	SendMessage(hWnd, WM_CLOSE, 0, 0);
		//}
		Keyboard_ProcessMessage(uMsg, wParam, lParam);

		break;
	case WM_CLOSE:
		hal::dout << "・ｽI・ｽ・ｽ・ｽm・ｽF\n" << std::endl;

		if (MessageBox(hWnd, "・ｽ{・ｽ・ｽ・ｽﾉ終・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｵ・ｽ・ｽ・ｽﾅゑｿｽ・ｽ・ｽ", "・ｽm・ｽF", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
		{
			//OK・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ黷ｽ
			DestroyWindow(hWnd);
		}
		else
		{
			//・ｽI・ｽ・ｽ・ｽﾈゑｿｽ
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	//・ｽK・ｽv・ｽﾌなゑｿｽ・ｽ・ｽ・ｽb・ｽZ・ｽ[・ｽW・ｽﾍ適・ｽ・ｽ・ｽﾉ擾ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ轤ｵ・ｽ・ｽ
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
