//==================================================
//main.cpp
//メイン処理
//作成日: 2025/05/09
//==================================================

#include <SDKDDKVer.h> // Windows SDK の既定バージョンを有効化
#define WIN32_LEAN_AND_MEAN	// 不要な Windows ヘッダを除外
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
// グローバル
//==================================

//#ifndef _DEBUG
int g_CountFPS;
char g_DebugStr[2048];
//#endif

#pragma comment(lib, "winmm.lib")

//==================================
// エントリ / メインループ
//==================================
//==================================  
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // タイミング管理用
    DWORD dwExecLastTime;
    DWORD dwFPSLastTime;
    DWORD dwCurrentTime;
    DWORD dwFrameCount;

    HRESULT dummy = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

    // ウィンドウクラスの登録
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    RegisterClass(&wc);

    // ウィンドウサイズの計算
    RECT window_rect = { 0,0,(LONG)SCREEN_WIDTH,(LONG)SCREEN_HEIGHT };
    DWORD window_style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
    AdjustWindowRect(&window_rect, window_style, FALSE);
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    // ウィンドウ作成
    HWND hWnd = CreateWindow(
        CLASS_NAME,
        WINDOW_CAPTION,
        window_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_width,
        window_height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    ShowWindow(hWnd, nCmdShow);

    // 初期化
    UpdateWindow(hWnd);
    Direct3D_Initialize(hWnd);
    Keyboard_Initialize();
    Mouse_Initialize(hWnd);
    Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
    Init();

    // メッセージループ
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    timeBeginPeriod(1);
    dwExecLastTime = dwFPSLastTime = timeGetTime();
    dwCurrentTime = dwFrameCount = 0;

    do
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            dwCurrentTime = timeGetTime();

            if ((dwCurrentTime - dwFPSLastTime) >= 1000)
            {
                g_CountFPS = dwFrameCount;
                dwFPSLastTime = dwCurrentTime;
                dwFrameCount = 0;
            }

            if ((dwCurrentTime - dwExecLastTime) >= ((float)1000 / FPS))
            {
                dwExecLastTime = dwCurrentTime;

                wsprintf(g_DebugStr, "DX21");
                wsprintf(&g_DebugStr[strlen(g_DebugStr)], "FPS: %d", g_CountFPS);
                SetWindowText(hWnd, g_DebugStr);

                Update();

                Direct3D_Clear();

                Draw();

                Direct3D_Present();

                keycopy();

                dwFrameCount++;
            }
        }

    } while (msg.message != WM_QUIT);

    Finalize();
    Shader_Finalize();
    Direct3D_Finalize();

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
    case WM_KEYDOWN:
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        break;
    case WM_CLOSE:
        hal::dout << "終了確認\n" << std::endl;

        if (MessageBox(hWnd, "本当に終了しますか?", "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
        {
            DestroyWindow(hWnd);
        }
        else
        {
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
