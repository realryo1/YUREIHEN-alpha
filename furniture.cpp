#include "furniture.h"
#include "Camera.h"
#include "shader.h"
#include "ghost.h"
#include "keyboard.h"
#include "define.h"

Furniture* g_Furniture[FURNITURE_NUM]{};

void Furniture_Initialize(void)
{
	// ----------------------------------------------------
	// 1:ロッキングチェア (Rocking Chair)
	// ----------------------------------------------------
	g_Furniture[0] = new Furniture(
		{ -5.0f, 0.0f, -5.0f },			// 左手前の部屋
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 45.0f, 0.0f },
		"asset\\model\\car_blue.fbx"
	);

	g_Furniture[1] = new Furniture(
		{ 5.0f, 0.0f, -5.0f },			// 右手前の部屋
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, -45.0f, 0.0f },
		"asset\\model\\rockingchair.fbx"
	);

	// ----------------------------------------------------
	// 2:木 (Tree)
	// ----------------------------------------------------

	// 中央広間の四隅
	g_Furniture[2] = new Furniture(
		{ -6.0f, 0.0f, 6.0f },			// 左奥
		{ 1.5f, 1.5f, 1.5f },			// 少し大きめに
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\tree.fbx"
	);

	g_Furniture[3] = new Furniture(
		{ 6.0f, 0.0f, 6.0f },			// 右奥
		{ 1.5f, 1.5f, 1.5f },
		{ 0.0f, 45.0f, 0.0f },
		"asset\\model\\tree.fbx"
	);

	g_Furniture[4] = new Furniture(
		{ -6.0f, 0.0f, -2.0f },			// 左手前
		{ 1.5f, 1.5f, 1.5f },
		{ 0.0f, 90.0f, 0.0f },
		"asset\\model\\tree.fbx"
	);

	g_Furniture[5] = new Furniture(
		{ 6.0f, 0.0f, -2.0f },			// 右手前
		{ 1.5f, 1.5f, 1.5f },
		{ 0.0f, 135.0f, 0.0f },
		"asset\\model\\tree.fbx"
	);


	// 共通設定: 全ての家具の着地高さを設定
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			g_Furniture[i]->SetGroundLevel(0.0f);
		}
	}
}

// ... (以下変更なし) ...

void Furniture_Update(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			g_Furniture[i]->Update();
		}
	}
}

void Furniture_Draw(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			g_Furniture[i]->Draw();
		}
	}
}

void Furniture_Finalize(void)
{
	for (int i = 0; i < FURNITURE_NUM; i++)
	{
		if (g_Furniture[i])
		{
			delete g_Furniture[i];
			g_Furniture[i] = nullptr;
		}
	}
}

Furniture* GetFurniture(int index)
{
	if (index >= 0 && index < FURNITURE_NUM)
	{
		return g_Furniture[index];
	}
	return nullptr;
}

//trueなら驚かせ中、falseならindexが不正
bool FurnitureScareStart(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		//ジャンプ中かどうか確認
		if (g_Furniture[index]->GetIsJumping())
		{
			hal::dout << "Furniture[" << index << "] is already jumping." << std::endl;
			return true; // すでにジャンプ中なら何もしない
		}

		g_Furniture[index]->JumpStart();
		hal::dout << "Furniture[" << index << "] jumped!" << std::endl;
		return true;
	}
	else
	{
		hal::dout << "Invalid furniture index: " << index << std::endl;
		return false;
	}
}

//trueなら終了、falseなら驚かせ中
bool FurnitureScareEnded(int index)
{
	if (index >= 0 && index < FURNITURE_NUM && g_Furniture[index])
	{
		//ジャンプ中かどうか確認
		return g_Furniture[index]->GetIsJumpEnded();
	}
	else
	{
		hal::dout << "Invalid furniture index: " << index << std::endl;
		return false;
	}
}