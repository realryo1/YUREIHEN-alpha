//--------------------------------------------------------------------------------------
// File: mouse.cpp
//
// 便利なマウスモジュール
//
//--------------------------------------------------------------------------------------
// 2020/02/11
//     DirectXTKより、なんちゃってC言語用にシェイプアップ改変
//
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------
#include "mouse.h"

#include <windowsx.h>
#include <assert.h>


#define SAFE_CLOSEHANDLE(h) if(h){CloseHandle(h); h = NULL;}


static Mouse_State        gState = {};
static HWND               gWindow = NULL;
static Mouse_PositionMode gMode = MOUSE_POSITION_MODE_ABSOLUTE;
static HANDLE             gScrollWheelValue = NULL;
static HANDLE             gRelativeRead = NULL;
static HANDLE             gAbsoluteMode = NULL;
static HANDLE             gRelativeMode = NULL;
static int                gLastX = 0;
static int                gLastY = 0;
static int                gRelativeX = INT32_MAX;
static int                gRelativeY = INT32_MAX;
static bool               gInFocus = true;


static void clipToWindow(void);


void Mouse_Initialize(HWND window)
{
    RtlZeroMemory(&gState, sizeof(gState));

    assert(window != NULL);

    RAWINPUTDEVICE Rid;
    Rid.usUsagePage = 0x01 /* HID_USAGE_PAGE_GENERIC */;
    Rid.usUsage = 0x02     /* HID_USAGE_GENERIC_MOUSE */;
    Rid.dwFlags = RIDEV_INPUTSINK;
    Rid.hwndTarget = window;
    RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE));

    gWindow = window;
    gMode = MOUSE_POSITION_MODE_ABSOLUTE;

    if (!gScrollWheelValue) { gScrollWheelValue = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE); }
    if (!gRelativeRead) { gRelativeRead = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE); }
    if (!gAbsoluteMode) { gAbsoluteMode = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE); }
    if (!gRelativeMode) { gRelativeMode = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE); }

    gLastX = 0;
    gLastY = 0;
    gRelativeX = INT32_MAX;
    gRelativeY = INT32_MAX;

    gInFocus = true;
}

void Mouse_Finalize(void)
{
    SAFE_CLOSEHANDLE(gScrollWheelValue);
    SAFE_CLOSEHANDLE(gRelativeRead);
    SAFE_CLOSEHANDLE(gAbsoluteMode);
    SAFE_CLOSEHANDLE(gRelativeMode);
}

void Mouse_GetState(Mouse_State* pState)
{
    memcpy(pState, &gState, sizeof(gState));
    pState->positionMode = gMode;

    DWORD Result = WaitForSingleObjectEx(gScrollWheelValue, 0, FALSE);
    if (Result == WAIT_FAILED) { return; }

    if (Result == WAIT_OBJECT_0) {

        pState->scrollWheelValue = 0;
    }

    if (pState->positionMode == MOUSE_POSITION_MODE_RELATIVE) {

        Result = WaitForSingleObjectEx(gRelativeRead, 0, FALSE);
        if (Result == WAIT_FAILED) { return; }

        if (Result == WAIT_OBJECT_0) {
            pState->x = 0;
            pState->y = 0;
        }
        else {
            SetEvent(gRelativeRead);
        }
    }
}

void Mouse_ResetScrollWheelValue(void)
{
    SetEvent(gScrollWheelValue);
}

void Mouse_SetMode(Mouse_PositionMode mode)
{
    if (gMode == mode)
        return;

    SetEvent((mode == MOUSE_POSITION_MODE_ABSOLUTE) ? gAbsoluteMode : gRelativeMode);

    assert(gWindow != NULL);

    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_HOVER;
    tme.hwndTrack = gWindow;
    tme.dwHoverTime = 1;
    TrackMouseEvent(&tme);
}

bool Mouse_IsConnected(void)
{
    return GetSystemMetrics(SM_MOUSEPRESENT) != 0;
}

bool Mouse_IsVisible(void)
{
    if (gMode == MOUSE_POSITION_MODE_RELATIVE) {
        return false;
    }

    CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
    GetCursorInfo(&info);

    return (info.flags & CURSOR_SHOWING) != 0;
}

void Mouse_SetVisible(bool visible)
{
    if (gMode == MOUSE_POSITION_MODE_RELATIVE) {
        return;
    }

    CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
    GetCursorInfo(&info);

    bool isVisible = (info.flags & CURSOR_SHOWING) != 0;

    if (isVisible != visible) {
        ShowCursor(visible);
    }
}

void Mouse_ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    HANDLE evts[3] = {
        gScrollWheelValue,
        gAbsoluteMode,
        gRelativeMode
    };

    switch (WaitForMultipleObjectsEx(_countof(evts), evts, FALSE, 0, FALSE))
    {
    case WAIT_OBJECT_0:
        gState.scrollWheelValue = 0;
        ResetEvent(evts[0]);
        break;

    case (WAIT_OBJECT_0 + 1):
    {
        gMode = MOUSE_POSITION_MODE_ABSOLUTE;
        ClipCursor(nullptr);

        POINT point;
        point.x = gLastX;
        point.y = gLastY;

        // リモートディスクトップに対応するために移動前にカーソルを表示する
        ShowCursor(TRUE);

        if (MapWindowPoints(gWindow, nullptr, &point, 1)) {
            SetCursorPos(point.x, point.y);
        }

        gState.x = gLastX;
        gState.y = gLastY;
    }
    break;

    case (WAIT_OBJECT_0 + 2):
    {
        ResetEvent(gRelativeRead);

        gMode = MOUSE_POSITION_MODE_RELATIVE;
        gState.x = gState.y = 0;
        gRelativeX = INT32_MAX;
        gRelativeY = INT32_MAX;

        ShowCursor(FALSE);

        clipToWindow();
    }
    break;

    case WAIT_FAILED:
        return;
    }

    switch (message)
    {
    case WM_ACTIVATEAPP:
        if (wParam) {

            gInFocus = true;

            if (gMode == MOUSE_POSITION_MODE_RELATIVE) {

                gState.x = gState.y = 0;
                ShowCursor(FALSE);
                clipToWindow();
            }
        }
        else {
            int scrollWheel = gState.scrollWheelValue;
            memset(&gState, 0, sizeof(gState));
            gState.scrollWheelValue = scrollWheel;
            gInFocus = false;
        }
        return;

    case WM_INPUT:
        if (gInFocus && gMode == MOUSE_POSITION_MODE_RELATIVE) {

            RAWINPUT raw;
            UINT rawSize = sizeof(raw);

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));

            if (raw.header.dwType == RIM_TYPEMOUSE) {

                if (!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {

                    gState.x = raw.data.mouse.lLastX;
                    gState.y = raw.data.mouse.lLastY;

                    ResetEvent(gRelativeRead);
                }
                else if (raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) {

                    // リモートディスクトップなどに対応
                    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                    int x = (int)((raw.data.mouse.lLastX / 65535.0f) * width);
                    int y = (int)((raw.data.mouse.lLastY / 65535.0f) * height);

                    if (gRelativeX == INT32_MAX) {
                        gState.x = gState.y = 0;
                    }
                    else {
                        gState.x = x - gRelativeX;
                        gState.y = y - gRelativeY;
                    }

                    gRelativeX = x;
                    gRelativeY = y;

                    ResetEvent(gRelativeRead);
                }
            }
        }
        return;


    case WM_MOUSEMOVE:
        break;

    case WM_LBUTTONDOWN:
        gState.leftButton = true;
        break;

    case WM_LBUTTONUP:
        gState.leftButton = false;
        break;

    case WM_RBUTTONDOWN:
        gState.rightButton = true;
        break;

    case WM_RBUTTONUP:
        gState.rightButton = false;
        break;

    case WM_MBUTTONDOWN:
        gState.middleButton = true;
        break;

    case WM_MBUTTONUP:
        gState.middleButton = false;
        break;

    case WM_MOUSEWHEEL:
        gState.scrollWheelValue += GET_WHEEL_DELTA_WPARAM(wParam);
        return;

    case WM_XBUTTONDOWN:
        switch (GET_XBUTTON_WPARAM(wParam))
        {
        case XBUTTON1:
            gState.xButton1 = true;
            break;

        case XBUTTON2:
            gState.xButton2 = true;
            break;
        }
        break;

    case WM_XBUTTONUP:
        switch (GET_XBUTTON_WPARAM(wParam))
        {
        case XBUTTON1:
            gState.xButton1 = false;
            break;

        case XBUTTON2:
            gState.xButton2 = false;
            break;
        }
        break;

    case WM_MOUSEHOVER:
        break;

    default:
        // マウスに対するメッセージは無かった…
        return;
    }

    if (gMode == MOUSE_POSITION_MODE_ABSOLUTE) {

        // すべてのマウスメッセージに対して新しい座標を取得する
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        gState.x = gLastX = xPos;
        gState.y = gLastY = yPos;
    }
}

void clipToWindow(void)
{
    assert(gWindow != NULL);

    RECT rect;
    GetClientRect(gWindow, &rect);

    POINT ul;
    ul.x = rect.left;
    ul.y = rect.top;

    POINT lr;
    lr.x = rect.right;
    lr.y = rect.bottom;

    MapWindowPoints(gWindow, NULL, &ul, 1);
    MapWindowPoints(gWindow, NULL, &lr, 1);

    rect.left = ul.x;
    rect.top = ul.y;

    rect.right = lr.x;
    rect.bottom = lr.y;

    ClipCursor(&rect);
}


