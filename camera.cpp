#include "camera.h"
#include "d3d11.h"
#include "DirectXMath.h"
using namespace DirectX;
#include "direct3d.h"
#include "define.h"
#include "shader.h"
#include "mouse.h"
#include "texture.h"
#include "debug_ostream.h"
#include "ghost.h"

static Camera* CameraObject;

namespace
{
	float NormalizeAngle180(float angle)
	{
		while (angle > 180.0f) angle -= 360.0f;
		while (angle < -180.0f) angle += 360.0f;
		return angle;
	}
}

void Camera_Initialize(void)
{
	CameraObject = new Camera(
		XMFLOAT3(0.0f, 0.0f, -5.0f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		45.0f,
		(float)DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT,
		1.0f,
		50.0f
	);
}

void Camera_Finalize(void)
{
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	Mouse_SetVisible(true);
	delete CameraObject;
}

void Camera_Update(void)
{
	CameraObject->Update();
}

void Camera_Draw(void)
{
}

void Camera_SetTargetPos(XMFLOAT3 targetPos)
{
	CameraObject->SetTargetPos(targetPos);
}

void Camera_LookAtPoint(XMFLOAT3 pointPos)
{
	if (!CameraObject) return;
	CameraObject->LookAtPoint(pointPos);
}

float Camera_GetYaw(void)
{
	return CameraObject->GetYaw();
}

void Camera_SetSensitivity(float sensitivity)
{
	CameraObject->SetSensitivity(sensitivity);
}

float Camera_GetSensitivity()
{
	return CameraObject->GetSensitivity();
}

Camera* GetCamera(void)
{
	return CameraObject;
}

void Camera_SetDistance(float dist)
{
	CameraObject->SetDistance(dist);
}

void Camera::Update()
{
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);

	// 絶対座標モード中にクリックで相対モードへ切り替え（カーソル固定）
	if (mouseState.positionMode == MOUSE_POSITION_MODE_ABSOLUTE)
	{
		if (mouseState.leftButton)
		{
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			Mouse_SetVisible(false);
		}
		return;
	}

	// 注視方向のイージング遷移中は、入力よりも補間を優先する
	if (m_isLookTransition)
	{
		const float dt = 1.0f / 60.0f;
		m_lookTransitionTime += dt;

		float t = (m_lookTransitionDuration > 0.0f)
			? (m_lookTransitionTime / m_lookTransitionDuration)
			: 1.0f;
		if (t > 1.0f) t = 1.0f;

		float smoothT = t * t * (3.0f - 2.0f * t);
		float yawDelta = NormalizeAngle180(m_lookEndYaw - m_lookStartYaw);

		m_yaw = NormalizeAngle180(m_lookStartYaw + yawDelta * smoothT);
		m_pitch = m_lookStartPitch + (m_lookEndPitch - m_lookStartPitch) * smoothT;

		if (t >= 1.0f)
		{
			m_isLookTransition = false;
			m_yaw = NormalizeAngle180(m_lookEndYaw);
			m_pitch = m_lookEndPitch;
		}

		m_lastPitch = m_pitch;
		m_lastYaw = m_yaw;
	}
	// フロア移行直後など gState に古い累積値が残る場合、指定フレーム数だけ入力を読み飛ばす
	else if (m_skipInputFrames > 0)
	{
		m_skipInputFrames--;
		// 入力はスキップするがカメラ位置計算は通常通り行う
	}
	else
	// ポーズ中は Camera_Update() 自体が呼ばれないため、モードチェックは不要
	// ただしフロア移行直後など gState に大きな累積値が残る場合があるため
	// 1フレームあたりの入力量を上限でクランプして異常回転を防ぐ
	{
		const int MAX_MOUSE_DELTA = 200;
		int mx = mouseState.x;
		int my = mouseState.y;
		if (mx >  MAX_MOUSE_DELTA) mx =  MAX_MOUSE_DELTA;
		if (mx < -MAX_MOUSE_DELTA) mx = -MAX_MOUSE_DELTA;
		if (my >  MAX_MOUSE_DELTA) my =  MAX_MOUSE_DELTA;
		if (my < -MAX_MOUSE_DELTA) my = -MAX_MOUSE_DELTA;

		m_yaw += static_cast<float>(mx) * MOUSE_SENSITIVITY * m_sensitivity * 2;
		m_pitch -= static_cast<float>(my) * MOUSE_SENSITIVITY * m_sensitivity * 2;

		if (m_pitch > PITCH_LIMIT_LOOK_UP)
		{
			m_pitch = PITCH_LIMIT_LOOK_UP;
		}
		else if (m_pitch < PITCH_LIMIT_LOOK_DOWN)
		{
			m_pitch = PITCH_LIMIT_LOOK_DOWN;
		}

		if (m_pitch != m_lastPitch || m_yaw != m_lastYaw)
		{
			m_lastPitch = m_pitch;
			m_lastYaw = m_yaw;
		}
	}

	// カメラ距離をターゲットへ滑らかに補間
	const float DIST_LERP_SPEED = 0.05f;
	m_distance += (m_targetDistance - m_distance) * DIST_LERP_SPEED;

	XMVECTOR targetVec = XMLoadFloat3(&m_targetPos);

	float pitchRad = XMConvertToRadians(m_pitch);
	float yawRad = XMConvertToRadians(m_yaw);

	float camX = -sinf(yawRad) * cosf(pitchRad) * m_distance;
	float camY = -sinf(pitchRad) * m_distance + 0.1f;
	float camZ = -cosf(yawRad) * cosf(pitchRad) * m_distance;

	XMVECTOR cameraPos = XMVectorAdd(targetVec, XMVectorSet(camX, camY, camZ, 0.0f));

	XMFLOAT3 newCameraPos;
	XMStoreFloat3(&newCameraPos, cameraPos);

	UpdateView(newCameraPos, m_targetPos);
}

void Camera::SetTargetPos(XMFLOAT3 targetPos)
{
	m_targetPos.x = targetPos.x;
	m_targetPos.y = targetPos.y + CAMERA_OFFSET_Y;
	m_targetPos.z = targetPos.z;
}

void Camera::LookAtPoint(const XMFLOAT3& pointPos, float duration)
{
	XMFLOAT3 dir = {
		pointPos.x - m_targetPos.x,
		pointPos.y - m_targetPos.y,
		pointPos.z - m_targetPos.z
	};

	float lenSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
	if (lenSq <= 0.000001f)
	{
		return;
	}

	float targetYaw = m_yaw;
	float flatLen = sqrtf(dir.x * dir.x + dir.z * dir.z);
	if (flatLen > 0.000001f)
	{
		targetYaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
	}

	float targetPitch = XMConvertToDegrees(atan2f(dir.y, (flatLen > 0.000001f) ? flatLen : 0.000001f));
	if (targetPitch > PITCH_LIMIT_LOOK_UP)
	{
		targetPitch = PITCH_LIMIT_LOOK_UP;
	}
	else if (targetPitch < PITCH_LIMIT_LOOK_DOWN)
	{
		targetPitch = PITCH_LIMIT_LOOK_DOWN;
	}

	m_lookStartYaw = NormalizeAngle180(m_yaw);
	m_lookStartPitch = m_pitch;
	m_lookEndYaw = NormalizeAngle180(targetYaw);
	m_lookEndPitch = targetPitch;
	m_lookTransitionTime = 0.0f;
	m_lookTransitionDuration = (duration > 0.0f) ? duration : 0.001f;
	m_isLookTransition = true;

	m_skipInputFrames = 0;
}