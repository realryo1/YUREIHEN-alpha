// UTF-8 BOM
#pragma execution_character_set("utf-8")

#include "UI_Tutorial_Internal.h"
#include "Tutorial_Object.h"
#include "define.h"

static void TutorialPage_EnablePianoOnlyPossessionPhase()
{
	SetTutorialPossessionOnlyPiano(true);
	SetTutorialScareEnabled(false);
	SetTutorialScareRequireBusterInRange(false);
}

static void TutorialPage_EnableBustersScarePhase()
{
	SetTutorialPossessionOnlyPiano(true);
	SetTutorialScareEnabled(true);
	SetTutorialScareRequireBusterInRange(true);
}

static void TutorialPage_EnableAllFurniturePossessionPhase()
{
	SetTutorialPossessionOnlyPiano(false);
	SetTutorialScareEnabled(true);
	SetTutorialScareRequireBusterInRange(false);
}

// ==========================================
// チュートリアルページ登録
// ここがあなたが触る部分です。
// AddPage / AddPage_Play / AddPage_Camera /
// SetCameraFocusPoint / SetTutorialMarker /
// SetTutorialBuster を使ってページを追加してください。
// ==========================================
void Tutorial_Pages_Init()
{
	//ここでピアノのみに憑依できるようにする
	//まだスペースキーで驚かせはできない状態	
	TutorialPage_EnablePianoOnlyPossessionPhase();

	// --- ウェルカムメッセージ ---
	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"遊んでくれてありがとう！「幽霊変」の遊び方を説明していくね！"
		});

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"まずは操作説明！",
		"[W][A][S][D] で移動、マウスで視点を動かせるよ",
		"前に進んで円盤に触れてみよう！"
		});

	// --- 移動操作テストプレイ ---
	SetTutorialMarker(true, { -4.5f, 0.5f, 17.0f });
	SetEnbanVisible(true);
	AddPage_Play(
		{ "[W][A][S][D] 移動・[マウス] 視点" },
		TutorialObject_GetEnbanTouchedPtr(),
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	// 壁番号1（ID:13）を無効化する（次ページのonEnterで実行される）
	SetTutorialWall(1, false);
	SetTutorialMarker(false);
	SetEnbanVisible(false);
	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"移動は完璧！",
		"次はゲームの目的、「敵を驚かせて追い払う！」について説明するね。"
		});

	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 300.0f,
		{ "これが家具の一つのピアノ。"
		  "[スペースキー]で憑依だよ！" },
		{ -13.0f, 3.0f, 16.5f }, { -22.0f, 0.5f, 16.5f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	// --- ピアノ憑依テストプレイ ---
	SetTutorialMarker(true, { -23.0f, 1.5f, 16.5f });
	SetCameraFocusPoint({ -15.0f, 1.0f, 16.5f });
	AddPage_Play(
		{ "[W][A][S][D]移動・[マウス]視点・[スペースキー]憑依" },
		TutorialObject_GetPianoPossessedPtr(),
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	//最初の壁を有効に
	SetTutorialWall(1, true);
	SetTutorialMarker(false);

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"憑依できたね！ おや？この影は？"
		});

	SetTutorialBuster(true, { -23.0f, BUSTERS_HEIGHT, 5.0f });

	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 300.0f,
		{ "うわっ！侵入者の「バスターず」だ！",
		  "近づいてきたら、[スペースキー]で驚かせよう！" },
		{ -23.0f, 1.0f, 10.0f }, { -23.0f, 1.0f, 5.0f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	// バスターズへの驚かせテストプレイ
	//このページで、バスターズが入ってきたらスペースキーで驚かせられるようにする

	SetTutorialBusterTarget({ -24.5f, 0.5f, 16.5f });
	TutorialPage_EnableBustersScarePhase();
	AddPage_Play(
		{ "近づいてくるまで待ち、[スペースキー]で驚かせる" },
		TutorialObject_GetBustersStunnedPtr(30),
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"ナイス！驚かせに成功したね！",
		});

	AddPage({ 1050.0f, 83.0f }, 50.0f, {
		"バスターズをうまく驚かせられると、右上の「恐怖ゲージ」が溜まっていくよ。",
		"MAXまでいくとステージクリア！次の階へ進もう",
		"全部なくなっちゃうと負けだから気を付けてね！",
		});

	AddPage({ 1163.0f, 201.0f }, 90.0f, {
		"あと、「恐怖コンボ」ってのも上がる。",
		"驚かせが連鎖すると、恐怖ゲージの上昇幅は増え、驚かせ範囲は広くなるよ！",
		});

	StartTutorialBusterExit({ -20.0f, BUSTERS_HEIGHT, -8.0f });

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"次は、家具できること！"
		"家具は大きく分けて「３種類」があるんだ。"
		});

	//幽撃
	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 100.0f }, 250.0f,
		{ "「霊撃」の家具",
		  "！のアイコンだよ。バスターずを驚かせられる！" },
		{ -20.0f, 2.0f, 12.5f }, { -10.0f, 2.0f, 12.5f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100.0f }
	);

	//誘引
	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 100.0f }, 220.0f,
		{ "「誘引」の家具",
		  "？のアイコンだよ。バスターずを引き寄せられる！",
		  "家具状態で移動できる。家具「霊撃」の近くに移動してみよう" },
		{ -13.5f, 3.0f, 14.5f }, { -13.5f, 0.2f, 10.5f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 170.0f }
	);

	//混乱
	AddPage_Camera({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 100.0f }, 220.0f,
		{ "「混乱」の家具",
		  "☆彡のアイコンだよ。バスターずの行動を止められる！",
		  "誘引で引き寄せて、混乱で止め、幽撃で驚かす！！" },
		{ -14.0f, 2.0f, 13.0f }, { -17.5f, -0.15f, 10.0f },
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT - 170.0f }
	);

	// 壁番号2（ID:14）を無効化する
	SetTutorialWall(2, false);
	// 壁番号3（ID:15）を無効化する
	SetTutorialWall(3, false);
	// 最後の部屋に通常の探索機能付きバスターズを出現させる
	StartNormalBusters({ -10.0f, BUSTERS_HEIGHT, -5.0f });
	//ここで全ての家具に憑依できるようにする
	TutorialPage_EnableAllFurniturePossessionPhase();

	AddPage({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }, 0.0f, {
		"このゲームの肝は、「？」で「！」のとこにおびき寄せ",
		"スペース連打でスコア稼ぐ！",
		"次の部屋で色々試してみて！",
		});
}
