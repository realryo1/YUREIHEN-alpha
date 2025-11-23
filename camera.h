#pragma once
#include "main.h"

enum CameraControlMode
{
	CAMERA_MODE_KEYBOARD,   // 既存のキーボード操作モード
	CAMERA_MODE_MOUSE_FPS   // FPSスタイルのマウス操作モード
};

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

public:
	Camera(
		XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, -5.0f),
		XMFLOAT3 atpos = XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT3 upvec = XMFLOAT3(0.0f, 1.0f, 0.0f),
		float fov = 45.0f,
		float aspect = SCREEN_WIDTH / SCREEN_HEIGHT,
		float near_ = 0.2f,//アンダーバーは定数回避
		float far_ = 100.0f)
		:m_Pos(pos), m_AtPos(atpos), m_UpVec(upvec),
		m_Fov(fov), m_Aspect(aspect), m_Near(near_), m_Far(far_)
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
};

void Camera_Initialize(void);
void Camera_Finalize(void);
void Camera_Update(void);
void Camera_Draw(void);
Camera* GetCamera(void);
