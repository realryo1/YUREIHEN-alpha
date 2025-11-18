#include "camera.h"
#include "d3d11.h"
#include "DirectXMath.h"
using namespace DirectX;
#include "direct3d.h"
#include "newShader.h"
#include "keyboard.h"
#include "mouse.h"
#include "texture.h"

#define ROTATE_Y_MAX (80.0f)
#define MOUSE_SENSITIVITY (0.15f)

static Camera* CameraObject;

XMFLOAT3 rotate;
XMFLOAT3 move;
float rotate_x_ruiseki = 0.0f;
float cameraDistance = 5.0f;

static CameraControlMode currentMode = CAMERA_MODE_KEYBOARD;
static Mouse_State previousMouseState = {};

// カーソルを確実に表示/非表示にする関数[web:54][web:55]
static void EnsureCursorVisible(bool visible)
{
    if (visible)
    {
        // カーソルが表示されるまでShowCursor(TRUE)を呼ぶ[web:52][web:53]
        int count = ShowCursor(TRUE);
        while (count < 0)
        {
            count = ShowCursor(TRUE);
        }
    }
    else
    {
        // カーソルが非表示になるまでShowCursor(FALSE)を呼ぶ[web:52][web:53]
        int count = ShowCursor(FALSE);
        while (count >= 0)
        {
            count = ShowCursor(FALSE);
        }
    }
}

void Camera_Initialize(void)
{
    CameraObject = new Camera();
    currentMode = CAMERA_MODE_KEYBOARD;
    Mouse_GetState(&previousMouseState);

    // 初期状態ではカーソルを表示[web:54]
    EnsureCursorVisible(true);
}

void Camera_Finalize(void)
{
    Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
    EnsureCursorVisible(true);

    delete CameraObject;
}

static void UpdateKeyboardMode(void)
{
    // ピッチ回転
    if (Keyboard_IsKeyDown(KK_UP))
    {
        rotate.x -= 0.05f;
    }
    else if (Keyboard_IsKeyDown(KK_DOWN))
    {
        rotate.x += 0.05f;
    }
    else
    {
        rotate.x *= 0.9f;
    }

    rotate_x_ruiseki += rotate.x;
    if (rotate_x_ruiseki > ROTATE_Y_MAX)
    {
        rotate_x_ruiseki = ROTATE_Y_MAX;
    }
    else if (rotate_x_ruiseki < -ROTATE_Y_MAX)
    {
        rotate_x_ruiseki = -ROTATE_Y_MAX;
    }

    // ヨー回転
    if (Keyboard_IsKeyDown(KK_RIGHT))
    {
        rotate.y += 0.05f;
    }
    else if (Keyboard_IsKeyDown(KK_LEFT))
    {
        rotate.y -= 0.05f;
    }
    else
    {
        rotate.y *= 0.9f;
    }

    // 移動処理
    if (Keyboard_IsKeyDown(KK_W))
    {
        move.z += 0.01f;
    }
    else if (Keyboard_IsKeyDown(KK_S))
    {
        move.z -= 0.01f;
    }
    else
    {
        move.z *= 0.9f;
    }

    if (Keyboard_IsKeyDown(KK_D))
    {
        move.x += 0.01f;
    }
    else if (Keyboard_IsKeyDown(KK_A))
    {
        move.x -= 0.01f;
    }
    else
    {
        move.x *= 0.9f;
    }

    if (Keyboard_IsKeyDown(KK_SPACE))
    {
        move.y += 0.01f;
    }
    else if (Keyboard_IsKeyDown(KK_LEFTSHIFT))
    {
        move.y -= 0.01f;
    }
    else
    {
        move.y *= 0.9f;
    }

    // 制限
    rotate.x = max(-1.5f, min(1.5f, rotate.x));
    rotate.y = max(-1.5f, min(1.5f, rotate.y));
    rotate.z = max(-1.5f, min(1.5f, rotate.z));
    move.x = max(-0.2f, min(0.2f, move.x));
    move.y = max(-0.2f, min(0.2f, move.y));
    move.z = max(-0.2f, min(0.2f, move.z));
}

static void UpdateMouseFPSMode(void)
{
    Mouse_State currentMouseState;
    Mouse_GetState(&currentMouseState);

    int mouseDeltaX = currentMouseState.x;
    int mouseDeltaY = currentMouseState.y;

    rotate.y = static_cast<float>(mouseDeltaX) * MOUSE_SENSITIVITY;
    rotate.x = static_cast<float>(mouseDeltaY) * MOUSE_SENSITIVITY;

    rotate_x_ruiseki += rotate.x;
    if (rotate_x_ruiseki > ROTATE_Y_MAX)
    {
        rotate_x_ruiseki = ROTATE_Y_MAX;
    }
    else if (rotate_x_ruiseki < -ROTATE_Y_MAX)
    {
        rotate_x_ruiseki = -ROTATE_Y_MAX;
    }

    // 移動処理
    if (Keyboard_IsKeyDown(KK_W))
    {
        move.z += 0.01f;
    }
    else if (Keyboard_IsKeyDown(KK_S))
    {
        move.z -= 0.01f;
    }
    else
    {
        move.z *= 0.9f;
    }

    if (Keyboard_IsKeyDown(KK_D))
    {
        move.x += 0.01f;
    }
    else if (Keyboard_IsKeyDown(KK_A))
    {
        move.x -= 0.01f;
    }
    else
    {
        move.x *= 0.9f;
    }

    if (Keyboard_IsKeyDown(KK_SPACE))
    {
        move.y += 0.01f;
    }
    else if (Keyboard_IsKeyDown(KK_LEFTSHIFT))
    {
        move.y -= 0.01f;
    }
    else
    {
        move.y *= 0.9f;
    }

    move.x = max(-0.2f, min(0.2f, move.x));
    move.y = max(-0.2f, min(0.2f, move.y));
    move.z = max(-0.2f, min(0.2f, move.z));

    previousMouseState = currentMouseState;
}

static void ApplyCameraTransform(void)
{
    XMFLOAT3 posFloat = CameraObject->GetPos();
    XMFLOAT3 atPosFloat = CameraObject->GetAtPos();

    XMVECTOR pos = XMLoadFloat3(&posFloat);
    XMVECTOR atPos = XMLoadFloat3(&atPosFloat);

    XMVECTOR forward = XMVectorSubtract(atPos, pos);
    forward = XMVectorSetY(forward, 0.0f);
    forward = XMVector3Normalize(forward);

    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));

    if (abs(rotate.y) > 0.001f)
    {
        float yawRad = XMConvertToRadians(rotate.y);
        XMMATRIX yawMat = XMMatrixRotationY(yawRad);

        XMVECTOR toTarget = XMVectorSubtract(atPos, pos);
        toTarget = XMVector3TransformNormal(toTarget, yawMat);
        atPos = XMVectorAdd(pos, toTarget);

        forward = XMVector3TransformNormal(forward, yawMat);
        right = XMVector3TransformNormal(right, yawMat);
    }

    XMVECTOR moveVec = XMVectorAdd(
        XMVectorScale(forward, move.z),
        XMVectorScale(right, move.x)
    );

    moveVec = XMVectorAdd(moveVec, XMVectorSet(0.0f, move.y, 0.0f, 0.0f));

    pos = XMVectorAdd(pos, moveVec);
    atPos = XMVectorAdd(atPos, moveVec);

    if (abs(rotate.x) > 0.001f)
    {
        float pitchRad = XMConvertToRadians(rotate.x);

        XMVECTOR toTarget = XMVectorSubtract(atPos, pos);
        XMMATRIX pitchMat = XMMatrixRotationAxis(right, pitchRad);
        toTarget = XMVector3TransformNormal(toTarget, pitchMat);
        atPos = XMVectorAdd(pos, toTarget);
    }

    if (currentMode == CAMERA_MODE_MOUSE_FPS)
    {
        rotate.x = 0.0f;
        rotate.y = 0.0f;
    }

    XMFLOAT3 newPos, newAtPos;
    XMStoreFloat3(&newPos, pos);
    XMStoreFloat3(&newAtPos, atPos);

    CameraObject->UpdateView(newPos, newAtPos);
}

void Camera_Update(void)
{
    // Tabキーでモード切り替え[web:55]
    if (Keyboard_IsKeyDownTrigger(KK_TAB))
    {
        if (currentMode == CAMERA_MODE_KEYBOARD)
        {
            currentMode = CAMERA_MODE_MOUSE_FPS;
            Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);

            // カーソルを確実に非表示[web:54][web:55]
            EnsureCursorVisible(false);

            rotate.x = 0.0f;
            rotate.y = 0.0f;
            rotate_x_ruiseki = 0.0f;
        }
        else
        {
            currentMode = CAMERA_MODE_KEYBOARD;
            Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);

            // カーソルを確実に表示[web:54][web:55]
            EnsureCursorVisible(true);

            rotate.x = 0.0f;
            rotate.y = 0.0f;
        }
    }

    if (currentMode == CAMERA_MODE_KEYBOARD)
    {
        UpdateKeyboardMode();
    }
    else if (currentMode == CAMERA_MODE_MOUSE_FPS)
    {
        UpdateMouseFPSMode();
    }

    ApplyCameraTransform();
}

// 描画処理（デバッグ用カメラ表示などに使用予定）
void Camera_Draw(void)
{
}

//  カメラオブジェクトを取得
Camera* GetCamera(void)
{
    return CameraObject;
}

CameraControlMode Camera_GetMode(void)
{
    return currentMode;
}

void Camera_SetMode(CameraControlMode mode)
{
    if (currentMode == mode)
        return;

    currentMode = mode;

    if (mode == CAMERA_MODE_MOUSE_FPS)
    {
        Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
        EnsureCursorVisible(false);
    }
    else
    {
        Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
        EnsureCursorVisible(true);
    }

    rotate.x = 0.0f;
    rotate.y = 0.0f;
}
