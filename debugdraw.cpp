#include "debugdraw.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"
#include "sprite3d.h"
#include "anim_sprite3d.h"

AnimSprite3D* g_AnimModelDraw = NULL;  // アニメーション対応モデル
Sprite3D* g_ComparisonModelDraw = NULL;  // 比較用モデル（通常のSprite3D）
static bool isUse = true;  // 処理の有効/無効を制御

void DebugDraw_Initialize(void)
{
	if (!isUse) return;
	
	// アニメーション対応モデル（cyancube.fbx）
	g_AnimModelDraw = new AnimSprite3D(
		{ 0.0f, 2.0f, 0.0f },			//位置
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\cyancube.fbx"	//モデルパス
	);
	
	// 初期アニメーション：rotate を再生
	bool success = g_AnimModelDraw->PlayAnimationByName("rotate", true);

	// 比較用モデル（通常のSprite3D）
	g_ComparisonModelDraw = new Sprite3D(
		{ 0.0f, 2.0f, 0.0f },			//位置（左側に配置）
		{ 1.0f, 1.0f, 1.0f },			//スケール
		{ 0.0f, 0.0f, 0.0f },			//回転（度）
		"asset\\model\\cyancube.fbx"	//モデルパス
	);

}

void DebugDraw_Update(void)
{
	if (!isUse) return;
	
	// アニメーション更新（毎フレーム呼び出し）
	const float dt = 1.0f / 60.0f;  // 60FPS想定
	g_AnimModelDraw->UpdateAnimation(dt);

	// キーボード入力でアニメーション切り替え
	if (Keyboard_IsKeyDownTrigger(KK_R))
	{
		// R キー：rotate アニメーション再生
		g_AnimModelDraw->PlayAnimationByName("rotate", true);
	}
	else if (Keyboard_IsKeyDownTrigger(KK_B))
	{
		// B キー：bounce アニメーション再生
		g_AnimModelDraw->PlayAnimationByName("bounce", true);
	}
	else if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		// Space キー：再生状態の切り替え
		if (g_AnimModelDraw->IsAnimationPlaying())
		{
			g_AnimModelDraw->PauseAnimation();
		}
		else
		{
			g_AnimModelDraw->ResumeAnimation();
		}
	}
}

void DebugDraw_Draw(void)
{
	if (!isUse) return;
	
	g_AnimModelDraw->Draw();  // アニメーション対応モデルを描画
	g_ComparisonModelDraw->Draw();  // 比較用モデルを描画
}

void DebugDraw_Finalize(void)
{
	if (!isUse) return;
	
	delete g_AnimModelDraw;
	delete g_ComparisonModelDraw;
}