// =========================================================
// fade.cpp フェード制御
// 
// 制作者:		日付：
// =========================================================
#include "main.h"
#include "fade.h"
#include "sprite.h"
#include "scene.h"
#include "texture.h"
#include "shader.h"
#include "direct3d.h"

// モジュール内の単一インスタンス（従来のグローバル構造体を置換）
static Fade* g_pFade = nullptr;

// =========================================================
// 初期化：インスタンス生成（Sprite のコンストラクタでテクスチャ読み込み）
// =========================================================
void Fade_Initialize(void)
{
	if (g_pFade == nullptr) {
		g_pFade = new Fade();
	}
}

// 外部ラッパー：モジュールの更新関数
void Fade_Update(void)
{
	if (g_pFade) {
		g_pFade->Update();
	}
}

// 描画：Sprite::Draw を使う（Sprite_Single_Draw を内部で呼ぶ）
void Fade_Draw(void)
{
	if (g_pFade) {
		g_pFade->Draw();
	}
}

// 終了処理：インスタンス削除（Sprite デストラクタでテクスチャ解放）
void Fade_Finalize(void)
{
	if (g_pFade) {
		delete g_pFade;
		g_pFade = nullptr;
	}
}

// 外部ラッパー：StartFade 呼び出し
void StartFade(SCENE ns)
{
	if (g_pFade) {
		g_pFade->StartFade(ns);
	}
}

// 外部ラッパー：GetState 呼び出し
FADESTAT GetFadeState(void)
{
	if (g_pFade) {
		return g_pFade->GetState();
	}
	return FADE_NONE;
}