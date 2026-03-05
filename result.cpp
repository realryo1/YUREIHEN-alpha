#include "result.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include "UI.h"
#include "sound.h"
#include "ScoreClient.h"
#include <sstream>
#include <iomanip>
using namespace DirectX;

// -------------------------------------------------------
// フェード演出フェーズ
// -------------------------------------------------------
enum ResultPhase
{
	PHASE_TIME_BIG = 0,		// Time: スコア下に大きくフェードイン
	PHASE_TIME_WAIT,		// Time: 大表示のまま待機（約1秒）
	PHASE_TIME_FADEOUT,		// Time: フェードアウト
	PHASE_TIME_SMALL,		// Time: 下部に小さくフェードイン
	PHASE_COMBO_BIG,		// Combo: スコア下に大きくフェードイン
	PHASE_COMBO_WAIT,		// Combo: 大表示のまま待機（約1秒）
	PHASE_COMBO_FADEOUT,	// Combo: フェードアウト
	PHASE_COMBO_SMALL,		// Combo: 下部に小さくフェードイン
	PHASE_KAKERU,			// kakeru2.png フェードイン
	PHASE_SCORE,			// スコアフェードイン
	PHASE_SPACE_GUIDE,		// 「SPACEでタイトルへ」フェードイン
	PHASE_DONE				// 全表示完了
};

// フェード速度（1フレームあたりのアルファ変化量）
static const float FADE_SPEED = 0.03f;
// 待機フレーム数（60fps × 1秒 = 60フレーム）
static const int   WAIT_FRAMES = 60;

// -------------------------------------------------------
// 座標・サイズ定数
// -------------------------------------------------------
// スコア画像の下・中央付近（大表示用）時間の結果
static const XMFLOAT2 TIME_BIG_POS = { SCREEN_WIDTH / 2.0f + 10, SCREEN_HEIGHT / 2.0f + 130.0f };//スコアの位置
static const XMFLOAT2 TIME_BIG_SIZE = { 50.0f, 50.0f };//スコアの大きさ
static const float    TIME_BIG_LABEL_FONT = 55.0f;//フォントサイズ
static const XMFLOAT2 TIME_BIG_LABEL_POS = { SCREEN_WIDTH / 2.0f - 130.0f, SCREEN_HEIGHT / 2.0f + 130.0f };//スコアのラベル位置

// 下部の小さい最終位置　時間の結果
static const XMFLOAT2 TIME_SMALL_POS = { SCREEN_WIDTH / 2.0f + 230.0f, SCREEN_HEIGHT / 2.0f - 13.0f };
static const XMFLOAT2 TIME_SMALL_SIZE = { 35.0f, 35.0f };
static const float    TIME_SMALL_SPACING = 30.0f;//桁間のスペース
static const float    TIME_SMALL_LABEL_FONT = 33.0f;//フォントサイズ
static const XMFLOAT2 TIME_SMALL_LABEL_POS = { SCREEN_WIDTH / 2.0f + 230.0f, SCREEN_HEIGHT / 2.0f - 50.0f };
// combo,スコアの下・中央付近（大表示用）
static const XMFLOAT2 COMBO_BIG_POS = { SCREEN_WIDTH / 2.0f + 10,  SCREEN_HEIGHT / 2.0f + 110.0f };
static const XMFLOAT2 COMBO_BIG_SIZE = { 70.0f, 70.0f };
static const float    COMBO_BIG_LABEL_FONT = 55.0f;
static const XMFLOAT2 COMBO_BIG_LABEL_POS = { SCREEN_WIDTH / 2.0f - 150.0f, SCREEN_HEIGHT / 2.0f + 130.0f };
// 下部の小さい最終位置 combo,スコアの下・中央付近（小表示用）
static const XMFLOAT2 COMBO_SMALL_POS = { SCREEN_WIDTH / 2.0f + 215.0f, SCREEN_HEIGHT / 2.0f + 95.0f };
static const XMFLOAT2 COMBO_SMALL_SIZE = { 35.0f, 35.0f };//スコアの大きさ
static const float    COMBO_SMALL_SPACING = 20.0f;//桁間のスペース
static const float    COMBO_SMALL_LABEL_FONT = 40.0f;//フォントサイズ（小表示用）
static const XMFLOAT2 COMBO_SMALL_LABEL_POS = { SCREEN_WIDTH / 2.0f + 230.0f, SCREEN_HEIGHT / 2.0f + 65.0f };
static const float    SCORE_LABEL_FONT = 40.0f;//フォントサイズ

// SPACEガイドの座標・サイズ定数
static const XMFLOAT2 SPACE_GUIDE_POS = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 200 };
static const float    SPACE_GUIDE_FONT = 35.0f;

// -------------------------------------------------------
// Sprite ポインタ
// -------------------------------------------------------
static Sprite* g_pResultSprite = nullptr;
static Sprite* g_pkyou1 = nullptr;
static Sprite* g_pkyou2 = nullptr;
static Sprite* g_pkyou3 = nullptr;
static Sprite* g_pResult_gakubuti = nullptr;
static Sprite* g_pkakeru = nullptr;
static Number* g_pTimeNum = nullptr;
static Number* g_pComboNum = nullptr;
static Number* g_pResultNum = nullptr;
static FontRenderer* g_pTimeLabelFont = nullptr;
static FontRenderer* g_pFloorLabelFont = nullptr;
static FontRenderer* g_pComboLabelFont = nullptr;		// Combo 大表示用ラベル
static FontRenderer* g_pComboLabelFontSmall = nullptr;	// Combo 小表示用ラベル（別サイズ）
static FontRenderer* g_pScoreFont = nullptr;
static FontRenderer* g_pSpaceGuideFont = nullptr;		// 「SPACEでタイトルへ」ガイド

static float g_pResultTime = 0.0f;
static int   g_pResultFloor = 1;
static int   g_pResultCombo = 1;

static SoundData* g_pBGM = nullptr;

// フェード演出用
static ResultPhase g_ResultPhase = PHASE_TIME_BIG;
static float       g_PhaseAlpha = 0.0f;
static int         g_WaitCounter = 0;	// 待機フレームカウンタ

// ★ スコア送信済みフラグ（1リザルト中に1回だけ送信するため）
static bool g_ScoreSent = false;

// -------------------------------------------------------
// ヘルパー関数
// -------------------------------------------------------
static int GetResultScore(void)
{
	return static_cast<int>(g_pResultTime) * g_pResultCombo;
}

static int GetDisplayTime(float time)
{
	int t = static_cast<int>(time);
	if (t >= 10 && t <= 99)
	{
		return t * 10;
	}
	return t;
}

// Time のアルファ・位置・サイズをまとめて適用
static void ApplyTimeBig(float alpha)
{
	if (g_pTimeLabelFont)
	{
		g_pTimeLabelFont->SetColor({ 0.0f, 0.0f, 0.0f, alpha });	// 黒
		g_pTimeLabelFont->SetPos(TIME_BIG_LABEL_POS);
	}
	if (g_pTimeNum)
	{
		g_pTimeNum->SetColor({ 1.0f, 1.0f, 1.0f, alpha });	// 画像色そのまま
		g_pTimeNum->SetPos(TIME_BIG_POS);
		g_pTimeNum->SetSize(TIME_BIG_SIZE);
		g_pTimeNum->SetDigitSpacing(TIME_BIG_SIZE.x - 10.0f);
	}
}

static void ApplyTimeSmall(float alpha)
{
	if (g_pTimeLabelFont)
	{
		g_pTimeLabelFont->SetColor({ 0.0f, 0.0f, 0.0f, alpha });	// 黒
		g_pTimeLabelFont->SetPos(TIME_SMALL_LABEL_POS);
	}
	if (g_pTimeNum)
	{
		g_pTimeNum->SetColor({ 1.0f, 1.0f, 1.0f, alpha });	// 画像色そのまま
		g_pTimeNum->SetPos(TIME_SMALL_POS);
		g_pTimeNum->SetSize(TIME_SMALL_SIZE);
		g_pTimeNum->SetDigitSpacing(TIME_SMALL_SPACING);
	}
}

// Combo のアルファ・位置・サイズをまとめて適用（大表示：g_pComboLabelFont を使用）
static void ApplyComboBig(float alpha)
{
	if (g_pComboLabelFont)
	{
		g_pComboLabelFont->SetColor({ 0.0f, 0.0f, 0.0f, alpha });	// 黒
		g_pComboLabelFont->SetPos(COMBO_BIG_LABEL_POS);
	}
	// 小表示用は非表示にしておく
	if (g_pComboLabelFontSmall)
	{
		g_pComboLabelFontSmall->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	}
	if (g_pComboNum)
	{
		g_pComboNum->SetColor({ 1.0f, 1.0f, 1.0f, alpha });	// 画像色そのまま
		g_pComboNum->SetPos(COMBO_BIG_POS);
		g_pComboNum->SetSize(COMBO_BIG_SIZE);
		g_pComboNum->SetDigitSpacing(COMBO_BIG_SIZE.x - 10.0f);
	}
}

// Combo 小表示（g_pComboLabelFontSmall を使用）
static void ApplyComboSmall(float alpha)
{
	// 大表示用は非表示にしておく
	if (g_pComboLabelFont)
	{
		g_pComboLabelFont->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	}
	if (g_pComboLabelFontSmall)
	{
		g_pComboLabelFontSmall->SetColor({ 0.0f, 0.0f, 0.0f, alpha });	// 黒
		g_pComboLabelFontSmall->SetPos(COMBO_SMALL_LABEL_POS);
	}
	if (g_pComboNum)
	{
		g_pComboNum->SetColor({ 1.0f, 1.0f, 1.0f, alpha });	// 画像色そのまま
		g_pComboNum->SetPos(COMBO_SMALL_POS);
		g_pComboNum->SetSize(COMBO_SMALL_SIZE);
		g_pComboNum->SetDigitSpacing(COMBO_SMALL_SPACING);
	}
}

static void SetScoreAlpha(float alpha)
{
	if (g_pResultNum)  g_pResultNum->SetColor({ 1.0f, 1.0f, 1.0f, alpha });	// 画像色そのまま
	if (g_pScoreFont)  g_pScoreFont->SetColor({ 0.0f, 0.0f, 0.0f, alpha });	// 黒
}

// kakeru2.png のアルファを適用
static void SetKakeruAlpha(float alpha)
{
	if (g_pkakeru) g_pkakeru->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
}

// -------------------------------------------------------
void Result_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
// フェード演出をリセット
	g_ResultPhase = PHASE_TIME_BIG;
	g_PhaseAlpha = 0.0f;
	g_WaitCounter = 0;
	g_ScoreSent = false;// スコア送信済みフラグをリセット

	// 背景
	g_pResultSprite = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ SCREEN_WIDTH , SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\yureihen\\Result\\yashiki.png"
	);

	g_pResult_gakubuti = new Sprite(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 20 },
		{ 795, 795 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\Result\\Result_gakubuti_Nofont.png"
	);

	g_pkakeru = new Sprite(
		{ SCREEN_WIDTH / 2.0f + 220, SCREEN_HEIGHT / 2.0f + 25 },
		{ 50, 50 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },	// 初期アルファ 0（フェードイン待機）
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\Result\\kakeru2.png"
	);

	Font_InitializeGlobalData();

	// タイム数字：最初は大きい位置・サイズで、アルファ 0
	g_pTimeNum = new Number(
		TIME_BIG_POS,
		TIME_BIG_SIZE,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		TIME_BIG_SIZE.x - 10.0f,
		2
	);

	// コンボ数字：最初は大きい位置・サイズで、アルファ 0
	g_pComboNum = new Number(
		COMBO_BIG_POS,
		COMBO_BIG_SIZE,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		COMBO_BIG_SIZE.x - 10.0f
	);
	g_pComboNum->SetShowX(true);

	// スコア数字：アルファ 0
	g_pResultNum = new Number(
		{ SCREEN_WIDTH / 2.0f + 60.0f, SCREEN_HEIGHT / 2.0f + 130.0f },
		{ 50.0f, 50.0f },
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\num.png",
		5, 3,
		40.0f
	);

	// タイマーラベル：最初は大きい位置で、アルファ 0
	g_pTimeLabelFont = new FontRenderer(
		TIME_BIG_LABEL_POS,
		TIME_BIG_LABEL_FONT,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		"Time:"
	);

	// コンボラベル（大表示用）：COMBO_BIG_LABEL_FONT サイズ、アルファ 0
	g_pComboLabelFont = new FontRenderer(
		COMBO_BIG_LABEL_POS,
		COMBO_BIG_LABEL_FONT,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		"Combo:"
	);

	// コンボラベル（小表示用）：COMBO_SMALL_LABEL_FONT サイズ、アルファ 0
	g_pComboLabelFontSmall = new FontRenderer(
		COMBO_SMALL_LABEL_POS,
		COMBO_SMALL_LABEL_FONT,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		"Combo:"
	);

	// Score: ラベル：スコア数値の左側・同じY座標
	g_pScoreFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f - 120.0f, SCREEN_HEIGHT / 2.0f + 130.0f },
		65.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		"Score:"
	);

	// SPACEガイド：Score:の下、アルファ 0
	g_pSpaceGuideFont = new FontRenderer(
		SPACE_GUIDE_POS,
		SPACE_GUIDE_FONT,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		"スペースキーでタイトルに戻る"
	);

	// 凶、恐、虚の絵
	g_pkyou1 = new Sprite(//nomal//0-400
		{ SCREEN_WIDTH / 2.0f - 30, SCREEN_HEIGHT / 2.0f - 10 },
		{ 250,250 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\Result\\Result_Normal.png"
	);
	g_pkyou2 = new Sprite(//good//401-1000
		{ SCREEN_WIDTH / 2.0f - 30, SCREEN_HEIGHT / 2.0f - 10 },
		{ 300,300 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\Result\\Result_Good.png"
	);
	g_pkyou3 = new Sprite(//excellent//1001-1200
		{ SCREEN_WIDTH / 2.0f - 30, SCREEN_HEIGHT / 2.0f - 10 },
		{ 300,300 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\yureihen\\Result\\Result_Excellent.png"
	);

	// サウンド
	g_pBGM = LoadMP3("asset/sound/bgm/HauntedHalloween.mp3");
	if (g_pBGM)
	{
		PlaySound(g_pBGM, true);
	}

	// 値を反映
	if (g_pTimeNum)
		g_pTimeNum->SetNumber(GetDisplayTime(g_pResultTime));

	if (g_pComboNum)
		g_pComboNum->SetNumber(g_pResultCombo);

	if (g_pResultNum)
		g_pResultNum->SetNumber(GetResultScore());
}

void Result_Update(void)
{
	if (g_ResultPhase != PHASE_DONE)
	{
		switch (g_ResultPhase)
		{
			// --- Time 大表示フェードイン ---
		case PHASE_TIME_BIG:
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			ApplyTimeBig(g_PhaseAlpha);
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_TIME_WAIT;
				g_WaitCounter = 0;
			}
			break;

			// --- Time 大表示 待機 ---
		case PHASE_TIME_WAIT:
			ApplyTimeBig(1.0f);	// アルファ1.0のまま維持
			g_WaitCounter++;
			if (g_WaitCounter >= WAIT_FRAMES)
			{
				g_ResultPhase = PHASE_TIME_FADEOUT;
				g_PhaseAlpha = 1.0f;
			}
			break;

			// --- Time フェードアウト ---
		case PHASE_TIME_FADEOUT:
			g_PhaseAlpha -= FADE_SPEED * 2.0f;
			if (g_PhaseAlpha < 0.0f) g_PhaseAlpha = 0.0f;
			ApplyTimeBig(g_PhaseAlpha);
			if (g_PhaseAlpha <= 0.0f)
			{
				ApplyTimeSmall(0.0f);
				g_ResultPhase = PHASE_TIME_SMALL;
				g_PhaseAlpha = 0.0f;
			}
			break;

			// --- Time 小表示フェードイン ---
		case PHASE_TIME_SMALL:
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			ApplyTimeSmall(g_PhaseAlpha);
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_COMBO_BIG;
				g_PhaseAlpha = 0.0f;
			}
			break;

			// --- Combo 大表示フェードイン ---
		case PHASE_COMBO_BIG:
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			ApplyComboBig(g_PhaseAlpha);
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_COMBO_WAIT;
				g_WaitCounter = 0;
			}
			break;

			// --- Combo 大表示 待機 ---
		case PHASE_COMBO_WAIT:
			ApplyComboBig(1.0f);	// アルファ1.0のまま維持
			g_WaitCounter++;
			if (g_WaitCounter >= WAIT_FRAMES)
			{
				g_ResultPhase = PHASE_COMBO_FADEOUT;
				g_PhaseAlpha = 1.0f;
			}
			break;

			// --- Combo フェードアウト ---
		case PHASE_COMBO_FADEOUT:
			g_PhaseAlpha -= FADE_SPEED * 2.0f;
			if (g_PhaseAlpha < 0.0f) g_PhaseAlpha = 0.0f;
			ApplyComboBig(g_PhaseAlpha);
			if (g_PhaseAlpha <= 0.0f)
			{
				ApplyComboSmall(0.0f);
				g_ResultPhase = PHASE_COMBO_SMALL;
				g_PhaseAlpha = 0.0f;
			}
			break;

			// --- Combo 小表示フェードイン ---
		case PHASE_COMBO_SMALL:
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			ApplyComboSmall(g_PhaseAlpha);
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_KAKERU;
				g_PhaseAlpha = 0.0f;
			}
			break;

			// --- kakeru2.png フェードイン ---
		case PHASE_KAKERU:
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			SetKakeruAlpha(g_PhaseAlpha);
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_SCORE;
				g_PhaseAlpha = 0.0f;
			}
			break;

			// --- スコアフェードイン ---
		case PHASE_SCORE:
			// ★ スコアが確定したタイミングで1回だけサーバーへ非同期送信
			if (!g_ScoreSent)
			{
				Score_SendToServerAsync(GetResultScore());
				g_ScoreSent = true;
			}
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			SetScoreAlpha(g_PhaseAlpha);
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_SPACE_GUIDE;
				g_PhaseAlpha = 0.0f;
			}
			break;

			// --- 「Press SPACE to Title」フェードイン ---
		case PHASE_SPACE_GUIDE:
			g_PhaseAlpha += FADE_SPEED;
			if (g_PhaseAlpha > 1.0f) g_PhaseAlpha = 1.0f;
			if (g_pSpaceGuideFont)
				g_pSpaceGuideFont->SetColor({ 1.0f, 1.0f, 1.0f, g_PhaseAlpha });
			if (g_PhaseAlpha >= 1.0f)
			{
				g_ResultPhase = PHASE_DONE;
			}
			break;

		default:
			break;
		}
	}

	// 演出完了後のみ投票シーンへ遷移
	if (g_ResultPhase == PHASE_DONE && Keyboard_IsKeyDown(KK_SPACE))
	{
		StartFade(SCENE_VOTE);
	}
}

void Result_Draw(void)
{
	g_pResultSprite->Draw();
	g_pResult_gakubuti->Draw();

	// かける画像を常時表示
	if (g_pkakeru) g_pkakeru->Draw();

	// スコアに応じた画像を表示
	int resultScore = GetResultScore();
	if (resultScore >= 0 && resultScore <= 400)
	{
		if (g_pkyou1) g_pkyou1->Draw();
	}
	else if (resultScore >= 401 && resultScore <= 1000)
	{
		if (g_pkyou2) g_pkyou2->Draw();
	}
	else if (resultScore >= 1001 && resultScore <= 1200)
	{
		if (g_pkyou3) g_pkyou3->Draw();
	}

	if (g_pTimeLabelFont)        g_pTimeLabelFont->Draw();
	if (g_pTimeNum)              g_pTimeNum->Draw();
	if (g_pComboLabelFont)       g_pComboLabelFont->Draw();
	if (g_pComboLabelFontSmall)  g_pComboLabelFontSmall->Draw();
	if (g_pScoreFont)            g_pScoreFont->Draw();
	if (g_pComboNum)             g_pComboNum->Draw();
	if (g_pResultNum)            g_pResultNum->Draw();
	if (g_pSpaceGuideFont)       g_pSpaceGuideFont->Draw();	// SPACEガイド
}

void Result_Finalize(void)
{
	if (g_pResultSprite) { delete g_pResultSprite;       g_pResultSprite = nullptr; }
	if (g_pTimeLabelFont) { delete g_pTimeLabelFont;      g_pTimeLabelFont = nullptr; }
	if (g_pkyou1) { delete g_pkyou1;              g_pkyou1 = nullptr; }
	if (g_pkyou2) { delete g_pkyou2;              g_pkyou2 = nullptr; }
	if (g_pkyou3) { delete g_pkyou3;              g_pkyou3 = nullptr; }
	if (g_pkakeru) { delete g_pkakeru;             g_pkakeru = nullptr; }
	if (g_pResult_gakubuti) { delete g_pResult_gakubuti;   g_pResult_gakubuti = nullptr; }
	if (g_pTimeNum) { delete g_pTimeNum;            g_pTimeNum = nullptr; }
	if (g_pFloorLabelFont) { delete g_pFloorLabelFont;    g_pFloorLabelFont = nullptr; }
	if (g_pComboLabelFont) { delete g_pComboLabelFont;    g_pComboLabelFont = nullptr; }
	if (g_pComboLabelFontSmall) { delete g_pComboLabelFontSmall; g_pComboLabelFontSmall = nullptr; }
	if (g_pComboNum) { delete g_pComboNum;           g_pComboNum = nullptr; }
	if (g_pResultNum) { delete g_pResultNum;          g_pResultNum = nullptr; }
	if (g_pScoreFont) { delete g_pScoreFont;          g_pScoreFont = nullptr; }
	if (g_pSpaceGuideFont) { delete g_pSpaceGuideFont;    g_pSpaceGuideFont = nullptr; }	// SPACEガイド解放

	if (g_pBGM)
	{
		StopSound(g_pBGM);
		UnloadSound(g_pBGM);
		g_pBGM = nullptr;
	}
}

void Result_SetTimerValue(float time)
{
	g_pResultTime = time;
	if (g_pTimeNum)  g_pTimeNum->SetNumber(GetDisplayTime(time));
	if (g_pResultNum) g_pResultNum->SetNumber(GetResultScore());
}

void Result_SetCombo(int combo)
{
	g_pResultCombo = combo;
	if (g_pComboNum)  g_pComboNum->SetNumber(combo);
	if (g_pResultNum) g_pResultNum->SetNumber(GetResultScore());
}