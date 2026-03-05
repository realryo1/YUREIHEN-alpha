#pragma once
#include "direct3d.h"
#include "define.h"
using namespace DirectX;

class Camera
{
protected:
	XMFLOAT3 m_Pos;
	XMFLOAT3 m_AtPos;
	XMFLOAT3 m_UpVec;

	XMMATRIX m_View;
	XMMATRIX m_Projection;

	float m_Fov; //視野角
	float m_Aspect;//アスペクト比
	float m_Near;//近くの限界値
	float m_Far;//遠くの限界値

	float m_pitch;
	float m_yaw;
	float m_lastPitch;
	float m_lastYaw;
	XMFLOAT3 m_targetPos;
	float m_sensitivity;
	float m_distance;
	float m_targetDistance;
	int m_skipInputFrames;
	bool m_isLookTransition;
	float m_lookTransitionTime;
	float m_lookTransitionDuration;
	float m_lookStartYaw;
	float m_lookStartPitch;
	float m_lookEndYaw;
	float m_lookEndPitch;

public:
	Camera(
		XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, -5.0f),
		XMFLOAT3 atpos = XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3 upvec = XMFLOAT3(0.0f, 1.0f, 0.0f),
		float fov = 45.0f,
		float aspect = DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT,
		float near_ = 0.2f,//アンダーバーは定数回避
		float far_ = 100.0f)
		:m_Pos(pos), m_AtPos(atpos), m_UpVec(upvec),
		m_Fov(fov), m_Aspect(aspect), m_Near(near_), m_Far(far_),
		m_pitch(0.0f), m_yaw(0.0f), m_lastPitch(0.0f), m_lastYaw(0.0f),
		m_targetPos(0.0f, 0.0f, 0.0f), m_sensitivity(0.1f),
		m_distance(CAMERA_DISTANCE), m_targetDistance(CAMERA_DISTANCE),
		m_skipInputFrames(0),
		m_isLookTransition(false), m_lookTransitionTime(0.0f), m_lookTransitionDuration(0.0f),
		m_lookStartYaw(0.0f), m_lookStartPitch(0.0f), m_lookEndYaw(0.0f), m_lookEndPitch(0.0f)
	{
		m_View = XMMatrixLookAtLH(
			XMVectorSet(m_Pos.x, m_Pos.y, m_Pos.z, 0.0f),
			XMVectorSet(m_AtPos.x, m_AtPos.y, m_AtPos.z, 0.0f),
			XMVectorSet(m_UpVec.x, m_UpVec.y, m_UpVec.z, 0.0f)
		);

		m_Projection = XMMatrixPerspectiveFovLH(
			XMConvertToRadians(m_Fov),
			m_Aspect,
			m_Near,
			m_Far
		);
	}

	void Update();

	void UpdateView(
		XMFLOAT3 pos,
		XMFLOAT3 atpos)
	{
		m_Pos = pos;
		m_AtPos = atpos;
		m_View = XMMatrixLookAtLH(
			XMVectorSet(m_Pos.x, m_Pos.y, m_Pos.z, 0.0f),
			XMVectorSet(m_AtPos.x, m_AtPos.y, m_AtPos.z, 0.0f),
			XMVectorSet(m_UpVec.x, m_UpVec.y, m_UpVec.z, 0.0f)
		);
	}

	XMFLOAT3 GetPos(void) const { return m_Pos; }
	XMFLOAT3 GetAtPos(void) const { return m_AtPos; }
	XMMATRIX GetView(void) const { return m_View; }
	XMMATRIX GetProjection(void) const { return m_Projection; }
	
	void SetTargetPos(XMFLOAT3 targetPos);
	void LookAtPoint(const XMFLOAT3& pointPos, float duration = 0.25f);
	float GetYaw(void) const { return m_yaw; }
	float GetPitch(void) const { return m_pitch; }
	void SetSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
	float GetSensitivity() const { return m_sensitivity; }
	void SetDistance(float dist) { m_targetDistance = dist; }
	void SkipNextInput(int frames = 2) { m_skipInputFrames = frames; }
};

// 注視対象を設定する関数（汎用化）
void Camera_Initialize(void);
void Camera_Finalize(void);
void Camera_Update(void);
void Camera_Draw(void);
float Camera_GetYaw(void);
void Camera_SetTargetPos(XMFLOAT3 targetPos);  // 注視対象位置を設定
void Camera_LookAtPoint(XMFLOAT3 pointPos);    // 現在の注視対象基準で向きを指定点へ合わせる
Camera* GetCamera(void);

//マウス感度設定
void Camera_SetSensitivity(float sensitivity);
float Camera_GetSensitivity();

// カメラ距離設定
void Camera_SetDistance(float dist);