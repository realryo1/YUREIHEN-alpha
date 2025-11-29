#include "title.h"
#include "main.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"

// ①Spriteのインスタンス、ポインタ用意
static Sprite* g_pTitleSprite = nullptr;

void Title_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ②各種初期化
	g_pTitleSprite = new Sprite(
		{ (SCREEN_WIDTH / 4.0f) * 3, SCREEN_HEIGHT / 2.0f },	//位置
		{ SCREEN_HEIGHT, SCREEN_HEIGHT },				//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },						//RGBA
		BLENDSTATE_NONE,								//BlendState
		L"asset\\texture\\1.png"						//テクスチャパス
	);


}

void Title_Update(void)
{
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDown(KK_SPACE))
	{
		g_pTitleSprite->AddRot(1.0f);
	}
	// ③適当な処理　アニメーションなどもここで
	if (Keyboard_IsKeyDown(KK_ENTER))
	{
		StartFade(SCENE_ANM_LOGO);
	}
}

void Title_Draw(void)
{
	// ④Drawするだけでいい！！！！！！！
	g_pTitleSprite->Draw();
}

void Title_Finalize(void)
{
	if (g_pTitleSprite) {
		delete g_pTitleSprite;
		g_pTitleSprite = nullptr;
	}
}

//正直、ここまで機能盛りだくさんなクラスはSOLID原則の"単一責任の原則"に反している気がするが……
//今回はいいや。やっとまともな進捗を得れたので。
//ってかこのコメントすらもCopilotが補完してくれるのウケる

