#include "debugdraw.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"
#include "anim_sprite3d.h"

AnimSprite3D* g_AnimModelDraw = NULL;  // アニメーション対応モデル
static bool isUse = true;  // 処理の有効/無効を制御

void DebugDraw_Initialize(void)
{
	if (!isUse) return;
	
	// アニメーション対応モデル（cyancube.fbx）
	g_AnimModelDraw = new AnimSprite3D(
		{ 0.0f, 0.0f, 0.0f },			//位置
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\kirbyanim.fbx"	//モデルパス
	);
}

void DebugDraw_Update(void)
{
	if (!isUse) return;
	
	// アニメーション更新（毎フレーム呼び出し）
	const float dt = 1.0f / 60.0f;  // 60FPS想定
	g_AnimModelDraw->UpdateAnimation(dt);

	// キーボード入力でアニメーション切り替え
	// カメラのy軸回転角度を取得（ヨー角をラジアンに変換）
	const float PI = 3.14159265f;
	float cameraYaw = Camera_GetYaw();
	float cameraYawRad = cameraYaw * PI / 180.0f;
	
	// 移動速度
	const float moveSpeed = 0.028f;
	XMFLOAT3 moveDirection = { 0.0f, 0.0f, 0.0f };
	bool isMoving = false;

	// キー入力に応じた移動方向の計算
	if (Keyboard_IsKeyDown(KK_UP))
	{
		// 上キー：カメラが向いている方向へ前進
		moveDirection.x += sinf(cameraYawRad) * moveSpeed;
		moveDirection.z += cosf(cameraYawRad) * moveSpeed;
		isMoving = true;
	}
	if (Keyboard_IsKeyDown(KK_DOWN))
	{
		// 下キー：カメラが向いている方向の逆方向へ後退
		moveDirection.x -= sinf(cameraYawRad) * moveSpeed;
		moveDirection.z -= cosf(cameraYawRad) * moveSpeed;
		isMoving = true;
	}
	if (Keyboard_IsKeyDown(KK_LEFT))
	{
		// 左キー：カメラが向いている方向の左方向へ移動
		moveDirection.x -= sinf(cameraYawRad + PI / 2.0f) * moveSpeed;
		moveDirection.z -= cosf(cameraYawRad + PI / 2.0f) * moveSpeed;
		isMoving = true;
	}
	if (Keyboard_IsKeyDown(KK_RIGHT))
	{
		// 右キー：カメラが向いている方向の右方向へ移動
		moveDirection.x -= sinf(cameraYawRad - PI / 2.0f) * moveSpeed;
		moveDirection.z -= cosf(cameraYawRad - PI / 2.0f) * moveSpeed;
		isMoving = true;
	}

	// モデルの位置を更新
	XMFLOAT3 currentPos = g_AnimModelDraw->GetPos();
	currentPos.x += moveDirection.x;
	currentPos.z += moveDirection.z;
	g_AnimModelDraw->SetPos(currentPos);

	// 移動方向に応じてモデルをy軸回転させる
	if (isMoving)
	{
		// 移動方向のy軸回転角度を計算（ラジアンから度数法へ変換）
		float targetAngle = atan2f(-moveDirection.x, -moveDirection.z) * 180.0f / PI;
		XMFLOAT3 currentRotation = g_AnimModelDraw->GetRot();
		
		// 角度の差分を計算（-180度～180度の範囲に正規化）
		float angleDiff = targetAngle - currentRotation.y;
		while (angleDiff > 180.0f) angleDiff -= 360.0f;
		while (angleDiff < -180.0f) angleDiff += 360.0f;
		
		// 目標角度に向けてスムーズに回転（回転速度は5度/フレーム）
		const float rotationSpeed = 5.0f;
		if (fabsf(angleDiff) > rotationSpeed)
		{
			currentRotation.y += (angleDiff > 0.0f ? rotationSpeed : -rotationSpeed);
		}
		else
		{
			currentRotation.y = targetAngle;
		}
		
		g_AnimModelDraw->SetRot(currentRotation);
		
		g_AnimModelDraw->PlayAnimationByName("run", true);
	}
	else
	{
		g_AnimModelDraw->PlayAnimationByName("wait", true);
	}

}

void DebugDraw_Draw(void)
{
	if (!isUse) return;
	
	g_AnimModelDraw->Draw();  // アニメーション対応モデルを描画
}

void DebugDraw_Finalize(void)
{
	if (!isUse) return;
	
	delete g_AnimModelDraw;
}