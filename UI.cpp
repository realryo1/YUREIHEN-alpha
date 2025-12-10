//UI.cpp

#include "UI.h"
#include "sprite.h"
#include "debug_ostream.h"
#include "keyboard.h"
#include "fade.h"
#include "UI_scarecombo.h"
#include "field.h"

// グローバル変数
static Timer* g_Clock = nullptr;
static Gauge* g_ScareGauge = nullptr;
Sprite* g_Reticle = nullptr;
static DWORD g_LastScoreUpdateTime = 0;

// ★追加: 階層表示用
static Number* g_FloorNumber = nullptr; // 数字 (1, 2, 3)
static Sprite* g_FloorTextF = nullptr;  // 文字 (F)
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

	// =======================================================
	// 階層表示UI
	// =======================================================

	// 時計の下あたりに配置 (X=120, Y=250 くらい)
	float floorPosX = CLOCK_POS_X;
	float floorPosY = CLOCK_POS_Y + 180.0f;

	// 階層番号 (1, 2, 3)
	g_FloorNumber = new Number(
		{ floorPosX - 20.0f, floorPosY },	// 位置 (少し左)
		{ 60.0f, 60.0f },					// サイズ
		{ 1.0f, 1.0f, 1.0f, 1.0f },			// 色
		BLENDSTATE_ALFA,					// BlendState
		L"asset\\texture\\num.png",			// テクスチャパス
		5, 3,								// 分割数X, Y
		40.0f								// 文字間隔
	);
	g_FloorNumber->SetNumber(1); // 初期値

	// 文字 "F"
	g_FloorTextF = new Sprite(
		{ floorPosX + 30.0f, floorPosY },	// 位置 (数字の右)
		{ 60.0f, 60.0f },					// サイズ
		0.0f,								// 回転
		{ 1.0f, 1.0f, 1.0f, 1.0f },			// 色
		BLENDSTATE_ALFA,					// BlendState
		L"asset\\texture\\floor_f.png"		// ★重要: "F"の画像を用意してください
	);
}


//----------------------------
//UI更新
//----------------------------
void UI_Update(void)
{
	if (Keyboard_IsKeyDown(KK_L))
	{
		SetScene(SCENE_ANM_LOSE);//Debug用に負けアニメーションへ直接飛ぶ
		return;

	}


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

	int currentFloor = Field_GetCurrentFloor() + 1;
	g_FloorNumber->SetNumber(currentFloor);
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

	if (g_FloorNumber) g_FloorNumber->Draw();
	if (g_FloorTextF) g_FloorTextF->Draw();
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

	if (g_FloorNumber) { delete g_FloorNumber; g_FloorNumber = nullptr; }
	if (g_FloorTextF) { delete g_FloorTextF; g_FloorTextF = nullptr; }
}

//恐怖ゲージ加算
void AddScareGauge(float value)
{
	if (g_ScareGauge)
	{
		g_ScareGauge->AddValue(value);
	}
}