#pragma execution_character_set("utf-8")

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "UI_PauseMenu.h"
#include "keyboard.h"
#include "mouse.h"
#include "camera.h"
#include "scene.h"
#include "define.h"
#include "sound.h"
#include "fade.h"
#include "UI_Tutorial.h"
#include <windows.h>
#include <string>
#include "shader.h"
#include "UI.h"
#include "game.h"
#include "field.h"

// ==========================================
// ポーズ画面用の変数
// ==========================================
static bool g_IsPause = false;
static int  g_PauseCursor = 0; // 0:ゲームに戻る, 1:音量, 2:マウス感度, 3:明るさ, 4:タイトル, 5:操作説明
static Sprite* g_pPauseBG = nullptr;
static ClickFont* g_pResumeButtonFont = nullptr;
static ClickFont* g_pVolumeButtonFont = nullptr;
static ClickFont* g_pMouseSensitivityButtonFont = nullptr;
static ClickFont* g_pBrightnessButtonFont = nullptr;
static ClickFont* g_pTitleButtonFont = nullptr;
static ClickFont* g_pTutorialImageButtonFont = nullptr; // 操作説明ボタン
static ClickFont* g_pSkipFloorButtonFont = nullptr; // 次の階へ進むボタン

// 左右矢印用フォント
static ClickFont* g_pLeftArrowFont = nullptr;
static ClickFont* g_pRightArrowFont = nullptr;

// 操作説明画像表示用
static bool g_IsShowingTutoImage = false;
static int  g_TutoImagePage = 0; // 0: tutoimage1, 1: tutoimage2
static Sprite* g_pTutoImage1 = nullptr;
static Sprite* g_pTutoImage2 = nullptr;
static ClickFont* g_pTutoImageCloseFont = nullptr;  // 「閉じる」ボタン
static ClickFont* g_pTutoImageNextFont = nullptr;   // 「次へ」ボタン
static ClickFont* g_pTutoImagePrevFont = nullptr;   // 「前へ」ボタン

// 設定値
static float g_Volume = 0.5f;
static float g_MouseSensitivity = 1.0f;
static float g_Brightness = 0.4f; // MainLightの環境光初期値に合わせる

// マウスカーソル状態フラグ
static bool g_PauseMouseStateChangedFlag = false;

// マウスクリック関連
struct MenuButton {
	float x, y;
	float width, height;
	int index;
};

static MenuButton g_MenuButtons[6] = {};

// 矢印表示オフセット定数
static const float ARROW_OFFSET_X = 150.0f; // 中央からの水平オフセット
static const float ARROW_HIT_MARGIN_X = 40.0f; // クリック判定のX軸マージン
static const float ARROW_HIT_MARGIN_Y = 30.0f; // クリック判定のY軸マージン

// ポーズメニューのレイアウト定数
static const float PAUSEMENU_BX = (SCREEN_WIDTH / 2.0f);
static const float PAUSEMENU_BY = (SCREEN_HEIGHT / 4.0f);
static const float PAUSEMENU_GAP = 70.0f;

// ==========================================

void UI_PauseMenu_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (g_pResumeButtonFont != nullptr) {
		return; // 既にフォントがあるなら何もしない
	}

	if (!pDevice || !pContext) return;
	
	// ポーズ用初期化
	g_IsPause = false;
	g_PauseCursor = 0;
	g_Volume = 0.5f;
	g_MouseSensitivity = 1.0f;
	g_Brightness = 0.4f;
	g_PauseMouseStateChangedFlag = false;
	g_IsShowingTutoImage = false;
	g_TutoImagePage = 0;

	// 初期音量を反映
	SetMasterVolume(g_Volume);

	// カメラに初期マウス感度を反映
	Camera_SetSensitivity(g_MouseSensitivity);

	// 背景
	g_pPauseBG = new Sprite(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0,
		{ 0,0,0,0.7f },
		BLENDSTATE_ALFA,
		L"asset/texture/fade.png");

	// ボタン
	float bx = PAUSEMENU_BX;
	float by = PAUSEMENU_BY;
	float gap = PAUSEMENU_GAP;
	XMFLOAT4 normal = { 1,1,1,1 };
	XMFLOAT4 hover = { 1.0f, 0.85f, 0.2f, 1.0f };

	// Resume (ゲームに戻る)
	g_pResumeButtonFont = new ClickFont({ bx, by }, 40.0f, 0.0f, normal, hover, "ゲームに戻る");
	g_pResumeButtonFont->SetHitSize({ 300.0f, 50.0f });
	g_pResumeButtonFont->SetOnClick([]() {
		g_IsPause = false;
		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		ShowCursor(FALSE);
		g_PauseMouseStateChangedFlag = false;
		UI_RefreshTimerForPause();
	});
	g_MenuButtons[0] = { bx - 150.0f, by - 25.0f, 300.0f, 50.0f, 0 };

	// Volume
	g_pVolumeButtonFont = new ClickFont({ bx, by + gap }, 40.0f, 0.0f, normal, hover, "音量");
	g_pVolumeButtonFont->SetHitSize({ 400.0f, 50.0f });
	g_pVolumeButtonFont->SetOnClick([]() {
		g_PauseCursor = 1;
	});
	g_MenuButtons[1] = { bx - 200.0f, by + gap - 25.0f, 400.0f, 50.0f, 1 };

	// MouseSensitivity
	g_pMouseSensitivityButtonFont = new ClickFont({ bx, by + gap * 2 }, 40.0f, 0.0f, normal, hover, "マウス感度");
	g_pMouseSensitivityButtonFont->SetHitSize({ 560.0f, 50.0f });
	g_pMouseSensitivityButtonFont->SetOnClick([]() {
		g_PauseCursor = 2;
	});
	g_MenuButtons[2] = { bx - 280.0f, by + gap * 2 - 25.0f, 560.0f, 50.0f, 2 };

	// Brightness
	g_pBrightnessButtonFont = new ClickFont({ bx, by + gap * 3 }, 40.0f, 0.0f, normal, hover, "明るさ");
	g_pBrightnessButtonFont->SetHitSize({ 400.0f, 50.0f });
	g_pBrightnessButtonFont->SetOnClick([]() {
		g_PauseCursor = 3;
	});
	g_MenuButtons[3] = { bx - 200.0f, by + gap * 3 - 25.0f, 400.0f, 50.0f, 3 };

	// Title
	g_pTitleButtonFont = new ClickFont({ bx, by + gap * 4 }, 40.0f, 0.0f, normal, hover, "タイトルへ戻る");
	g_pTitleButtonFont->SetHitSize({ 360.0f, 50.0f });
	g_pTitleButtonFont->SetOnClick([]() {
		g_IsPause = false;
		StartFade(SCENE_TITLE);
	});
	g_MenuButtons[4] = { bx - 180.0f, by + gap * 4 - 25.0f, 360.0f, 50.0f, 4 };

	// 操作説明
	g_pTutorialImageButtonFont = new ClickFont({ bx, by + gap * 5 }, 40.0f, 0.0f, normal, hover, "操作説明");
	g_pTutorialImageButtonFont->SetHitSize({ 300.0f, 50.0f });
	g_pTutorialImageButtonFont->SetOnClick([]() {
		g_IsShowingTutoImage = true;
		g_TutoImagePage = 0;
	});
	g_MenuButtons[5] = { bx - 150.0f, by + gap * 5 - 25.0f, 300.0f, 50.0f, 5 };

	// 次の階へ進む
	g_pSkipFloorButtonFont = new ClickFont({ bx, by + gap * 6 }, 40.0f, 0.0f, normal, hover, "次の階へ進む（非常用）");
	g_pSkipFloorButtonFont->SetHitSize({ 360.0f, 50.0f });
	g_pSkipFloorButtonFont->SetOnClick([]() {
		if (Field_GetCurrentFloor() > 0)
		{
			g_IsPause = false;
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			ShowCursor(FALSE);
			g_PauseMouseStateChangedFlag = false;
			UI_RefreshTimerForPause();

			Game_ForceSkipFloor();
		}
	});

	// 左右矢印フォント（初期は透明）
	g_pLeftArrowFont = new ClickFont({ bx - ARROW_OFFSET_X, by + gap }, 70.0f, 0.0f, { 1,1,1,0 }, { 1,1,1,0 }, "←");
	g_pLeftArrowFont->SetHitSize({ ARROW_HIT_MARGIN_X * 2.0f, ARROW_HIT_MARGIN_Y * 2.0f });
	g_pLeftArrowFont->SetOnClick([]() {
		if (g_PauseCursor == 1) {
			g_Volume -= 0.02f;
			if (g_Volume < 0.0f) g_Volume = 0.0f;
			SetMasterVolume(g_Volume);
		}
		else if (g_PauseCursor == 2) {
			g_MouseSensitivity -= 0.05f;
			if (g_MouseSensitivity < 0.1f) g_MouseSensitivity = 0.1f;
			Camera_SetSensitivity(g_MouseSensitivity);
		}
		else if (g_PauseCursor == 3) {
			g_Brightness -= 0.05f;
			if (g_Brightness < 0.0f) g_Brightness = 0.0f;
		}
	});

	g_pRightArrowFont = new ClickFont({ bx + ARROW_OFFSET_X, by + gap }, 70.0f, 0.0f, { 1,1,1,0 }, { 1,1,1,0 }, "→");
	g_pRightArrowFont->SetHitSize({ ARROW_HIT_MARGIN_X * 2.0f, ARROW_HIT_MARGIN_Y * 2.0f });
	g_pRightArrowFont->SetOnClick([]() {
		if (g_PauseCursor == 1) {
			g_Volume += 0.02f;
			if (g_Volume > 1.0f) g_Volume = 1.0f;
			SetMasterVolume(g_Volume);
		}
		else if (g_PauseCursor == 2) {
			g_MouseSensitivity += 0.05f;
			if (g_MouseSensitivity > 2.0f) g_MouseSensitivity = 2.0f;
			Camera_SetSensitivity(g_MouseSensitivity);
		}
		else if (g_PauseCursor == 3) {
			g_Brightness += 0.05f;
			if (g_Brightness > 1.0f) g_Brightness = 1.0f;
		}
	});

	// 操作説明画像スプライト（少し小さく・上寄せ）
	// 画像サイズ: 幅1100 x 高さ580、画面上部に寄せる（中心Y = 580/2 + 20 = 310）
	static const float TUTO_IMG_W  = 1100.0f;
	static const float TUTO_IMG_H  = 580.0f;
	static const float TUTO_IMG_CX = SCREEN_WIDTH  / 2.0f;
	static const float TUTO_IMG_CY = 20.0f + TUTO_IMG_H / 2.0f; // 上端から20px

	g_pTutoImage1 = new Sprite(
		{ TUTO_IMG_CX, TUTO_IMG_CY },
		{ TUTO_IMG_W, TUTO_IMG_H },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset/texture/tutoimage1.png"
	);

	g_pTutoImage2 = new Sprite(
		{ TUTO_IMG_CX, TUTO_IMG_CY },
		{ TUTO_IMG_W, TUTO_IMG_H },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset/texture/tutoimage2.png"
	);

	// ボタンY座標（画像の下端 + 余白40px）
	static const float TUTO_BTN_Y = 20.0f + TUTO_IMG_H + 50.0f;

	// 操作説明画像の「閉じる」ボタン（中央）
	g_pTutoImageCloseFont = new ClickFont(
		{ SCREEN_WIDTH / 2.0f, TUTO_BTN_Y }, 36.0f, 0.0f,
		{ 1,1,1,1 }, { 1.0f, 0.85f, 0.2f, 1.0f }, "閉じる"
	);
	g_pTutoImageCloseFont->SetHitSize({ 200.0f, 50.0f });
	g_pTutoImageCloseFont->SetOnClick([]() {
		g_IsShowingTutoImage = false;
	});

	// 操作説明画像の「次へ」ボタン（右寄り、1ページ目のみ表示）
	g_pTutoImageNextFont = new ClickFont(
		{ SCREEN_WIDTH / 2.0f + 250.0f, TUTO_BTN_Y }, 36.0f, 0.0f,
		{ 1,1,1,1 }, { 1.0f, 0.85f, 0.2f, 1.0f }, "次へ →"
	);
	g_pTutoImageNextFont->SetHitSize({ 200.0f, 50.0f });
	g_pTutoImageNextFont->SetOnClick([]() {
		if (g_TutoImagePage < 1) g_TutoImagePage++;
	});

	// 操作説明画像の「前へ」ボタン（左寄り、2ページ目のみ表示）
	g_pTutoImagePrevFont = new ClickFont(
		{ SCREEN_WIDTH / 2.0f - 250.0f, TUTO_BTN_Y }, 36.0f, 0.0f,
		{ 1,1,1,1 }, { 1.0f, 0.85f, 0.2f, 1.0f }, "← 前へ"
	);
	g_pTutoImagePrevFont->SetHitSize({ 200.0f, 50.0f });
	g_pTutoImagePrevFont->SetOnClick([]() {
		if (g_TutoImagePage > 0) g_TutoImagePage--;
	});
}

void UI_PauseMenu_Finalize(void)
{
	if (g_pPauseBG) {
		delete g_pPauseBG;
		g_pPauseBG = nullptr;
	}

	if (g_pResumeButtonFont) {
		delete g_pResumeButtonFont;
		g_pResumeButtonFont = nullptr;
	}

	if (g_pVolumeButtonFont) {
		delete g_pVolumeButtonFont;
		g_pVolumeButtonFont = nullptr;
	}

	if (g_pMouseSensitivityButtonFont) {
		delete g_pMouseSensitivityButtonFont;
		g_pMouseSensitivityButtonFont = nullptr;
	}

	if (g_pBrightnessButtonFont) {
		delete g_pBrightnessButtonFont;
		g_pBrightnessButtonFont = nullptr;
	}

	if (g_pTitleButtonFont) {
		delete g_pTitleButtonFont;
		g_pTitleButtonFont = nullptr;
	}

	if (g_pTutorialImageButtonFont) {
		delete g_pTutorialImageButtonFont;
		g_pTutorialImageButtonFont = nullptr;
	}

	if (g_pSkipFloorButtonFont) {
		delete g_pSkipFloorButtonFont;
		g_pSkipFloorButtonFont = nullptr;
	}

	// 左右矢印フォント
	if (g_pLeftArrowFont) {
		delete g_pLeftArrowFont;
		g_pLeftArrowFont = nullptr;
	}
	if (g_pRightArrowFont) {
		delete g_pRightArrowFont;
		g_pRightArrowFont = nullptr;
	}

	// 操作説明画像
	if (g_pTutoImage1) { delete g_pTutoImage1; g_pTutoImage1 = nullptr; }
	if (g_pTutoImage2) { delete g_pTutoImage2; g_pTutoImage2 = nullptr; }
	if (g_pTutoImageCloseFont) { delete g_pTutoImageCloseFont; g_pTutoImageCloseFont = nullptr; }
	if (g_pTutoImageNextFont) { delete g_pTutoImageNextFont; g_pTutoImageNextFont = nullptr; }
	if (g_pTutoImagePrevFont) { delete g_pTutoImagePrevFont; g_pTutoImagePrevFont = nullptr; }
}

// マウスクリックでボタン判定を行うヘルパー関数
static bool IsMouseInButton(const MenuButton& button, const Mouse_State& mouseState)
{
	return (mouseState.x >= button.x && mouseState.x <= button.x + button.width &&
			mouseState.y >= button.y && mouseState.y <= button.y + button.height);
}

void UI_PauseMenu_Update(void)
{
	// ESCキーでポーズ開始/終了
	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		// 操作説明画像表示中なら閉じる
		if (g_IsPause && g_IsShowingTutoImage)
		{
			g_IsShowingTutoImage = false;
			return;
		}

		g_IsPause = !g_IsPause;
		g_PauseCursor = 0;

		// ポーズ状態が変わったときにマウスカーソル状態を管理
		if (g_IsPause)
		{
			// ポーズ開始：マウスを絶対座標モード・表示
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
			ShowCursor(TRUE);
			g_PauseMouseStateChangedFlag = true;
		}
		else
		{
			// ポーズ終了：マウスを相対モード・非表示に戻す
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
			ShowCursor(FALSE);
			g_PauseMouseStateChangedFlag = false;
			UI_RefreshTimerForPause();
		}
	}

	if (!g_IsPause) return;

	// 操作説明画像表示中の更新
	if (g_IsShowingTutoImage)
	{
		if (g_pTutoImageCloseFont) g_pTutoImageCloseFont->Update();
		if (g_TutoImagePage == 0 && g_pTutoImageNextFont) g_pTutoImageNextFont->Update();
		if (g_TutoImagePage == 1 && g_pTutoImagePrevFont) g_pTutoImagePrevFont->Update();
		return;
	}

	// マウス状態取得（ClickFontのUpdate前に必要）
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);

	// ClickFont 更新（ホバー色・クリック）
	if (g_pResumeButtonFont) g_pResumeButtonFont->Update();
	if (g_pVolumeButtonFont) g_pVolumeButtonFont->Update();
	if (g_pMouseSensitivityButtonFont) g_pMouseSensitivityButtonFont->Update();
	if (g_pBrightnessButtonFont) g_pBrightnessButtonFont->Update();
	if (g_pTitleButtonFont) g_pTitleButtonFont->Update();
	if (g_pTutorialImageButtonFont) g_pTutorialImageButtonFont->Update();
	if (g_pSkipFloorButtonFont && Field_GetCurrentFloor() > 0) g_pSkipFloorButtonFont->Update();

	// 矢印は表示中のみUpdate（クリック判定もここに含む）
	if (g_PauseCursor == 1 || g_PauseCursor == 2 || g_PauseCursor == 3)
	{
		if (g_pLeftArrowFont) g_pLeftArrowFont->Update();
		if (g_pRightArrowFont) g_pRightArrowFont->Update();
	}

	// マウスホバーで選択状態を変更
	if (g_pResumeButtonFont && g_pResumeButtonFont->IsHover()) g_PauseCursor = 0;
	else if (g_pVolumeButtonFont && g_pVolumeButtonFont->IsHover()) g_PauseCursor = 1;
	else if (g_pMouseSensitivityButtonFont && g_pMouseSensitivityButtonFont->IsHover()) g_PauseCursor = 2;
	else if (g_pBrightnessButtonFont && g_pBrightnessButtonFont->IsHover()) g_PauseCursor = 3;
	else if (g_pTitleButtonFont && g_pTitleButtonFont->IsHover()) g_PauseCursor = 4;
	else if (g_pTutorialImageButtonFont && g_pTutorialImageButtonFont->IsHover()) g_PauseCursor = 5;
	else if (g_pSkipFloorButtonFont && g_pSkipFloorButtonFont->IsHover() && Field_GetCurrentFloor() > 0) g_PauseCursor = 6;

	// テキスト更新（クリック後の反映）
	if (g_pVolumeButtonFont)
	{
		int volumePercent = (int)(g_Volume * 100);
		std::string volumeText = "音量：" + std::to_string(volumePercent) + " ";
		g_pVolumeButtonFont->SetText(volumeText);
	}

	if (g_pMouseSensitivityButtonFont)
	{
		int sensitivityPercent = (int)(g_MouseSensitivity * 100);
		std::string sensitivityText = "マウス感度：" + std::to_string(sensitivityPercent) + " ";
		g_pMouseSensitivityButtonFont->SetText(sensitivityText);
	}

	if (g_pBrightnessButtonFont)
	{
		int brightnessPercent = (int)(g_Brightness * 100);
		std::string brightnessText = "明るさ：" + std::to_string(brightnessPercent) + " ";
		g_pBrightnessButtonFont->SetText(brightnessText);
	}

	// ボタン色はClickFont内蔵（通常色/ホバー色）に任せる

	// 上下キーで項目選択
	int maxCursor = (Field_GetCurrentFloor() > 0) ? 6 : 5;
	if (Keyboard_IsKeyDownTrigger(KK_UP))
	{
		g_PauseCursor--;
		if (g_PauseCursor < 0) g_PauseCursor = maxCursor;
	}
	if (Keyboard_IsKeyDownTrigger(KK_DOWN))
	{
		g_PauseCursor++;
		if (g_PauseCursor > maxCursor) g_PauseCursor = 0;
	}

	// 左右キーで値変更（音量・マウス感度・明るさ選択中のみ）
	if (Keyboard_IsKeyDown(KK_LEFT))
	{
		if (g_PauseCursor == 1)
		{
			g_Volume -= 0.02f;
			if (g_Volume < 0.0f) g_Volume = 0.0f;
			SetMasterVolume(g_Volume);
		}
		else if (g_PauseCursor == 2)
		{
			g_MouseSensitivity -= 0.05f;
			if (g_MouseSensitivity < 0.1f) g_MouseSensitivity = 0.1f;
			Camera_SetSensitivity(g_MouseSensitivity);
		}
		else if (g_PauseCursor == 3)
		{
			g_Brightness -= 0.05f;
			if (g_Brightness < 0.0f) g_Brightness = 0.0f;
		}
	}
	if (Keyboard_IsKeyDown(KK_RIGHT))
	{
		if (g_PauseCursor == 1)
		{
			g_Volume += 0.02f;
			if (g_Volume > 1.0f) g_Volume = 1.0f;
			SetMasterVolume(g_Volume);
		}
		else if (g_PauseCursor == 2)
		{
			g_MouseSensitivity += 0.05f;
			if (g_MouseSensitivity > 2.0f) g_MouseSensitivity = 2.0f;
			Camera_SetSensitivity(g_MouseSensitivity);
		}
		else if (g_PauseCursor == 3)
		{
			g_Brightness += 0.05f;
			if (g_Brightness > 1.0f) g_Brightness = 1.0f;
		}
	}

	switch (g_PauseCursor) {
	case 0:
		break;
	case 1:
		// 矢印の表示更新（音量時のみ表示）
		if (g_pLeftArrowFont) {
			g_pLeftArrowFont->SetPos({ PAUSEMENU_BX - ARROW_OFFSET_X, PAUSEMENU_BY + PAUSEMENU_GAP });
			if (g_Volume <= 0.0f) {
				g_pLeftArrowFont->SetColor({ 1,1,1,0 });
			}
			else {
				g_pLeftArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		if (g_pRightArrowFont) {
			g_pRightArrowFont->SetPos({ PAUSEMENU_BX + ARROW_OFFSET_X, PAUSEMENU_BY + PAUSEMENU_GAP });
			if (g_Volume >= 1.0f) {
				g_pRightArrowFont->SetColor({ 1,1,1,0 });
			}
			else {
				g_pRightArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		break;
	case 2:
		// 矢印の表示更新（マウス感度時のみ表示）
		if (g_pLeftArrowFont) {
			g_pLeftArrowFont->SetPos({ PAUSEMENU_BX - ARROW_OFFSET_X, PAUSEMENU_BY + PAUSEMENU_GAP * 2 });
			if (g_MouseSensitivity <= 0.1f) {
				g_pLeftArrowFont->SetColor({ 1,1,1,0 });
			}
			else {
				g_pLeftArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		if (g_pRightArrowFont) {
			g_pRightArrowFont->SetPos({ PAUSEMENU_BX + ARROW_OFFSET_X, PAUSEMENU_BY + PAUSEMENU_GAP * 2 });
			if (g_MouseSensitivity >= 2.0f) {
				g_pRightArrowFont->SetColor({ 1,1,1,0 });
			}
			else {
				g_pRightArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		break;
	case 3:
		// 矢印の表示更新（明るさ時のみ表示）
		if (g_pLeftArrowFont) {
			g_pLeftArrowFont->SetPos({ PAUSEMENU_BX - ARROW_OFFSET_X, PAUSEMENU_BY + PAUSEMENU_GAP * 3 });
			if (g_Brightness <= 0.0f) {
				g_pLeftArrowFont->SetColor({ 1,1,1,0 });
			}
			else {
				g_pLeftArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		if (g_pRightArrowFont) {
			g_pRightArrowFont->SetPos({ PAUSEMENU_BX + ARROW_OFFSET_X, PAUSEMENU_BY + PAUSEMENU_GAP * 3 });
			if (g_Brightness >= 1.0f) {
				g_pRightArrowFont->SetColor({ 1,1,1,0 });
			}
			else {
				g_pRightArrowFont->SetColor({ 1,1,1,1 });
			}
		}
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	}
}

void UI_PauseMenu_Draw(void)
{
	if (!g_IsPause) return;

	// 操作説明画像を表示中
	if (g_IsShowingTutoImage)
	{
		// ゲームUIと混ざらないよう背景を先に描画する
		if (g_pPauseBG) g_pPauseBG->Draw();

		// FontRenderer::Draw()がShader_SetMaterialColorを書き換えるため、
		// Sprite描画前に白にリセットする
		Shader_SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		if (g_TutoImagePage == 0 && g_pTutoImage1) g_pTutoImage1->Draw();
		if (g_TutoImagePage == 1 && g_pTutoImage2) g_pTutoImage2->Draw();
		if (g_pTutoImageCloseFont) g_pTutoImageCloseFont->Draw();
		if (g_TutoImagePage == 0 && g_pTutoImageNextFont) g_pTutoImageNextFont->Draw();
		if (g_TutoImagePage == 1 && g_pTutoImagePrevFont) g_pTutoImagePrevFont->Draw();
		return;
	}

	if (g_pPauseBG) g_pPauseBG->Draw();
	if (g_pResumeButtonFont) g_pResumeButtonFont->Draw();
	if (g_pVolumeButtonFont) g_pVolumeButtonFont->Draw();
	if (g_pMouseSensitivityButtonFont) g_pMouseSensitivityButtonFont->Draw();
	if (g_pBrightnessButtonFont) g_pBrightnessButtonFont->Draw();
	if (g_pTitleButtonFont) g_pTitleButtonFont->Draw();
	if (g_pTutorialImageButtonFont) g_pTutorialImageButtonFont->Draw();
	if (g_pSkipFloorButtonFont && Field_GetCurrentFloor() > 0) g_pSkipFloorButtonFont->Draw();

	// 左右矢印（音量・マウス感度・明るさ変更用）
	if (g_PauseCursor == 1 || g_PauseCursor == 2 || g_PauseCursor == 3)
	{
		if (g_pLeftArrowFont) g_pLeftArrowFont->Draw();
		if (g_pRightArrowFont) g_pRightArrowFont->Draw();
	}
}

bool UI_PauseMenu_IsPaused(void)
{
	return g_IsPause;
}

void UI_PauseMenu_SetPause(bool isPause)
{
	g_IsPause = isPause;
	g_PauseCursor = 0;

	if (g_IsPause)
	{
		Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		ShowCursor(TRUE);
		g_PauseMouseStateChangedFlag = true;
	}
	else
	{
		Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		ShowCursor(FALSE);
		g_PauseMouseStateChangedFlag = false;
		UI_RefreshTimerForPause();
	}
}

float UI_PauseMenu_GetBrightness(void)
{
	return g_Brightness;
}
