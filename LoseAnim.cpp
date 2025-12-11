#include "sprite.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "LoseAnim.h"
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Lose Animation (負けアニメーション)
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sprite* g_LoseBgSprite = nullptr;		// 背景（Losehaikei）
Sprite* g_LoseGhostSprite = nullptr;	// ゴーストエフェクト（LoseGhost）
Sprite* g_LoseInkSprite = nullptr;		// インク画像（Loseink）
static DWORD g_LoseStartTime = 0;
static const float GHOST_APPEAR_TIME = 0.8f;	// ゴースト表示開始時間（秒）
static const float GHOST_FADE_DURATION = 0.6f; // ゴーストのフェードインにかける時間（秒）
static const float INK_DROP_START_TIME = 1.2f;	// インク降下開始時間（秒）
static const float INK_DROP_DURATION = 1.0f;	// インク降下時間（秒）
static float g_LoseInkInitialY = 0.0f;	// インク画像の初期Y座標

void Animation_Lose_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 背景スプライト
	g_LoseBgSprite = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },	// 位置
		{ 1280, 1080},								// サイズ
		0.0f,										// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },				// 色
		BLENDSTATE_ALFA,							// BlendState
		L"asset\\yureihen\\Losehaikei.png"		// テクスチャパス
	);

	// ゴーストスプライト
	float ghostNativePixelSize = 1028.0f;
	float ghostScaleFactor = 0.4f;
	float ghostDisplaySize = ghostNativePixelSize * ghostScaleFactor;
	
	g_LoseGhostSprite = new Sprite(
		{ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.45f },	// 位置（光の中央）
		{ ghostDisplaySize, ghostDisplaySize },				// サイズ 
		0.0f,											// 回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.0f },					// 色（初期は完全透明）
		BLENDSTATE_ALFA,								// BlendState
		L"asset\\yureihen\\LoseGhost.png"				// テクスチャパス
	);

	// インクの開始Yを画面外上部に設定（画像高さの半分分上）
	g_LoseInkInitialY = -(1028.0f * 0.5f);

	// インク画像（初期位置を画面外上部にして、初期は透明にしておく）
	g_LoseInkSprite = new Sprite(
		{ SCREEN_WIDTH / 2, g_LoseInkInitialY },
		{ 1028, 1028 }, // 画像本来のサイズ
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f }, // 初期は透明（表示されないようにする）
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\LoseAnimeLogo.png"
	);

	g_LoseStartTime = timeGetTime();
}

void Animation_Lose_Update(void)
{
	DWORD currentTime = timeGetTime();
	DWORD elapsedTime = currentTime - g_LoseStartTime;
	float elapsedSeconds = elapsedTime / 1000.0f;

	// ========================
	// ゴーストのフェードイン処理（開始遅延 + 指定時間でフェード）
	// ========================
	if (elapsedSeconds < GHOST_APPEAR_TIME)
	{
		// ゴースト表示開始前は完全透明
		g_LoseGhostSprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}
	else
	{
		float fadeElapsed = elapsedSeconds - GHOST_APPEAR_TIME;
		if (fadeElapsed < GHOST_FADE_DURATION)
		{
			// 進行度 0..1
			float progress = fadeElapsed / GHOST_FADE_DURATION;

			// イージング（緩やかに出現させる）
			float eased = progress * progress; // ease-in

			g_LoseGhostSprite->SetColor({ 1.0f, 1.0f, 1.0f, eased });
		}
		else
		{
			// フェード完了
			g_LoseGhostSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}

	// ========================
	// インク画像の降下アニメーション
	// ========================
	if (elapsedSeconds >= INK_DROP_START_TIME)
	{
		float inkElapsedTime = elapsedSeconds - INK_DROP_START_TIME;

		if (inkElapsedTime <= INK_DROP_DURATION)
		{
			// 降下の進行度（0.0 ～ 1.0）
			float progress = inkElapsedTime / INK_DROP_DURATION;

			// イージング関数（加速度的に落ちる効果）
			float easedProgress = progress * progress;

			// 画面上部から画面中央付近までの移動距離
			float startY = g_LoseInkInitialY;			// 開始位置（画面外）
			float endY = SCREEN_HEIGHT * 0.35f;			// 終了位置（画面中央より少し上）
			float currentY = startY + (endY - startY) * easedProgress;

			// インク画像のY座標を更新（SetPosY を使用）
			g_LoseInkSprite->SetPosY(currentY);

			// フェードイン効果
			float inkAlpha = progress;
			g_LoseInkSprite->SetColor({ 1.0f, 1.0f, 1.0f, inkAlpha });
		}
		else
		{
			// 降下完了後は完全表示・位置固定
			g_LoseInkSprite->SetPosY(SCREEN_HEIGHT * 0.35f);
			g_LoseInkSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}

	// ENTERキーでゲームへ戻る
	if (Keyboard_IsKeyDownTrigger(KK_ENTER))
	{
		StartFade(SCENE_GAME);
	}
}

void Animation_Lose_Draw(void)
{
	g_LoseBgSprite->Draw();		// 背景を先に描画
	g_LoseInkSprite->Draw();	// インク画像を描画
	g_LoseGhostSprite->Draw();	// ゴーストを最後に描画（前面）
}

void Animation_Lose_Finalize(void)
{
	delete g_LoseBgSprite;
	delete g_LoseGhostSprite;
	delete g_LoseInkSprite;
}