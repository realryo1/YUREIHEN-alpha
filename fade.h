// =========================================================
// fade.h フェード制御
// 
// 制作者:		日付：
// =========================================================
#ifndef _FADE_H_
#define _FADE_H_

#include "sprite.h"
#include "scene.h"

// =========================================================
// 列挙体宣言
// =========================================================
enum FADESTAT
{
	FADE_NONE = 0,
	FADE_OUT,
	FADE_IN,
	FADE_MAX
};

// =========================================================
// Spriteを継承したFadeクラス
// =========================================================
class Fade : public Sprite
{
private:
	FADESTAT m_State;
	SCENE m_NextScene;

public:
	// コンストラクタ：Spriteのパラメータを引き継ぐ
	Fade()
		: Sprite(
			XMFLOAT2(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f),	// 位置
			XMFLOAT2(SCREEN_WIDTH, SCREEN_HEIGHT),					// サイズ
			0.0f,													// 回転
			XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f),						// 色（RGBA）アルファ値0
			BLENDSTATE_ALFA,										// ブレンドステート
			L"asset\\texture\\fade.png"								// テクスチャパス
		),
		m_State(FADE_NONE),
		m_NextScene(SCENE_NONE)
	{
	}

	~Fade()
	{
		// Spriteのデストラクタが自動的にテクスチャを解放
	}

	// 更新処理
	void Update()
	{
		switch (m_State)
		{
		case FADE_NONE:
			// 何もしない
			break;
		case FADE_OUT:
			m_Color.w += 0.04f;
			if (m_Color.w >= 1.0f)
			{
				m_Color.w = 1.0f;
				m_State = FADE_IN;
				if (m_NextScene != SCENE_NONE)
				{
					SetScene(m_NextScene);
				}
			}
			break;
		case FADE_IN:
			m_Color.w -= 0.04f;
			if (m_Color.w <= 0.0f)
			{
				m_Color.w = 0.0f;
				m_State = FADE_NONE;
			}
			break;
		default:
			break;
		}
	}

	// フェード開始
	void StartFade(SCENE next = SCENE_NONE)
	{
		if (m_State == FADE_NONE)
		{
			m_Color.w = 0.0f;
			m_State = FADE_OUT;
			m_NextScene = next;
		}
	}

	// ゲッター
	FADESTAT GetState() const { return m_State; }
};

// =========================================================
// モジュール関数（従来のC形式インターフェース互換）
// =========================================================
void Fade_Initialize(void);
void Fade_Update(void);
void Fade_Draw(void);
void Fade_Finalize(void);
void StartFade(SCENE ns = SCENE_NONE);
FADESTAT GetFadeState(void);

#endif