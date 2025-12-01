//UI.cpp

#include "UI.h"
#include "sprite.h"
#include "debug_ostream.h"
#include "keyboard.h"
#include "fade.h"
#include "UI_scarecombo.h"

// グローバル変数
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;
static DWORD g_LastScoreUpdateTime = 0;

//----------------------------
//UI初期化
//----------------------------
void UI_Initialize(void)
{
	// 時計の作成
	g_Clock = new Timer(
		{ CLOCK_POS_X, CLOCK_POS_Y },	// 位置
		{ CLOCK_SIZE, CLOCK_SIZE },		// サイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },		// 色
		BLENDSTATE_ALFA,				// BlendState
		L"asset\\texture\\clock.png",	// テクスチャパス
		2, 1,							// 分割数X, Y
		CLOCK_MIN, CLOCK_MAX			// 最小時間、最大時間
	);

	// 恐怖ゲージの作成
	g_ScareGauge = new Gauge(
		{ SCREEN_WIDTH - 270.0f, 70.0f },				// 位置
		{ GAUGE_SIZE, GAUGE_SIZE },						// サイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },						// 色
		BLENDSTATE_ALFA,								// BlendState
		L"asset\\texture\\gauge.png",					// テクスチャパス
		3, 1,											// 分割数X, Y
		0.0f, 100.0f,									// 最小値、最大値
		2, 0											// ゲージテクスチャ番号、背景テクスチャ番号
	);

	// 仮のレティクル
	g_Reticle = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },	//位置
		{ 30.0f, 30.0f },								//サイズ
		0.0f,											//回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.5f },						//RGBA
		BLENDSTATE_ALFA,								//BlendState
		L"asset\\texture\\grass.png"					//テクスチャパス
	);

	// 恐怖コンボの初期化
	UI_ScareCombo_Initialize();
}

//----------------------------
//UI更新
//----------------------------
void UI_Update(void)
{
	//恐怖ゲージが最大なら勝利シーンへ移行（デバッグ用）
	if (g_ScareGauge->GetValue() >= g_ScareGauge->GetMaxValue())
	{
		StartFade(SCENE_ANM_WIN);
	}

	// 時計が終わるか、スコアが０なら負けアニメーションへ移行
	if (g_Clock->Update() || g_ScareGauge->GetValue() <= 0.0f)
	{
		//デバッグ用
		hal::dout << "負けアニメーションを再生します" << std::endl;
		StartFade(SCENE_ANM_LOSE);
	}

	// 恐怖コンボの更新
	UI_ScareCombo_Update();
}

//----------------------------
//UI描画
//----------------------------
void UI_Draw(void)
{
	g_Clock->Draw(); // 時計を描画
	g_ScareGauge->Draw(); // ゲージを描画
	//g_Reticle->Draw();
	UI_ScareCombo_Draw(); // 恐怖コンボを描画
}

//----------------------------
//UI終了
//----------------------------
void UI_Finalize(void)
{
	delete g_Clock;
	delete g_ScareGauge;
	delete g_Reticle;
	UI_ScareCombo_Finalize();
}

//恐怖ゲージ加算
void AddScareGauge(float value)
{
	if (g_ScareGauge)
	{
		g_ScareGauge->AddValue(value);
	}
}