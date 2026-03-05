#pragma once

//==============================================================================
// プロジェクト全体の定数定義ファイル
//==============================================================================

#define CLASS_NAME L"幽霊変 - Violisun"
#define WINDOW_CAPTION L"幽霊変 - Violisun"
#define SCREEN_WIDTH (1280.0f)	// UI要素の配置に使う（いままで通り）
#define SCREEN_HEIGHT (720.0f)	// UI要素の配置に使う（いままで通り）
#define DRAW_SCREEN_WIDTH  (3840.0f)   // （最終的な描画解像度）　実際の配置には使わない！！！
#define DRAW_SCREEN_HEIGHT (2160.0f)   // （最終的な描画解像度）　実際の配置には使わない！！！
#define DRAW_SCALE_X (DRAW_SCREEN_WIDTH  / SCREEN_WIDTH)   // 描画倍率X
#define DRAW_SCALE_Y (DRAW_SCREEN_HEIGHT / SCREEN_HEIGHT)  // 描画倍率Y
#define WIN32_LEAN_AND_MEAN	//32bitアプリには不要な情報を無視
#define FPS (60)

//=== Sound 関連定数 ===
#define SOUND_BGM_VOLUME (0.2f)
#define SOUND_SE_VOLUME  (0.7f)

#if defined(_DEBUG)
//=== デバッグ関連定数 ===
#define STOP_TIMER_BUSTER (false) //trueならタイマーとバスターズのupdateを停止させる
#define DIRECT_START (true) //trueならgameシーンから直接開始する
#define DEBUG_DRAW (false) //trueならdebugdraw機能を有効にする
#define DEBUG_BUSTERS_ROTATION (false) //trueならバスターズの回転デバッグモード（左右矢印で回転、移動・検知・勝敗判定停止）
#define DEBUG_MODEL_SCENE (false) //trueならデバッグモデルビューアシーンから直接開始する（modelフォルダ内の全fbx/glbを自動表示）
#else
//==================================
//          ここは触らない
//==================================
#define STOP_TIMER_BUSTER (false) 
#define DIRECT_START (false) 
#define DEBUG_DRAW (false)
#define DEBUG_BUSTERS_ROTATION (false)
#define DEBUG_MODEL_SCENE (false)
//==================================
//          ここは触らない
//==================================
#endif

//=== Tutorial 関連定数 ===
#define TUTORIAL_SKIP_FRAME (2) // シーン遷移後、このフレーム数だけ通常更新してからチュートリアルを開始

//=== Tutorial スキップバー関連定数 ===
#define TUT_SKIPBAR_CENTER_X  (SCREEN_WIDTH  - 120.0f) // バー中心X（右下基準）
#define TUT_SKIPBAR_CENTER_Y  (SCREEN_HEIGHT -  80.0f) // バー中心Y
#define TUT_SKIPBAR_WIDTH     (150.0f)                  // バーの全幅
#define TUT_SKIPBAR_HEIGHT    (10.0f)                   // バーの高さ
#define TUT_SKIP_LABEL_OFFSET_Y (-30.0f)                // バーからラベルへのYオフセット
#define TUT_GUIDE_OFFSET_Y     ( 35.0f)                 // バーから[SPACE]案内へのYオフセット
#define TUT_PAGECOUNT_OFFSET_Y (-65.0f)                 // バーからページ数表示へのYオフセット

//=== TutorialMarker 関連定数 ===
#define TUTORIAL_MARKER_SIZE        (1.4f)  // 矢印ビルボードのサイズ（幅・高さ）
#define TUTORIAL_MARKER_BOB_AMP     (0.25f) // バウンス振幅（上下の振れ幅）
#define TUTORIAL_MARKER_BOB_SPEED   (3.0f)  // バウンス速度（rad/s）
#define TUTORIAL_MARKER_BASE_HEIGHT (2.0f)  // 地面からの基本高さ

//=== フィールド関連定数 ===
#define MAP_FLOORS (3)
#define START_FLOOR (3)// 階の数字をそのまま入れる（使用時に-1）
#define END_FLOOR (1) // この階で恐怖ゲージMAXクリア

//=== Camera 関連定数 ===
#define PITCH_LIMIT_LOOK_UP    (25.0f)   // 上を見る限界（カメラが下がる限界）: 床埋まり防止
#define PITCH_LIMIT_LOOK_DOWN  (-60.0f)  // 下を見る限界（カメラが上がる限界）: 天井埋まり防止
#define MOUSE_SENSITIVITY (0.15f)
#define CAMERA_DISTANCE (6.0f)  // カメラ距離
#define CAMERA_DISTANCE_TRANSFORM (9.0f) // 変身時のカメラ距離
#define CAMERA_OFFSET_Y (1.5f)  // 注視点のオフセット

//=== Furniture 関連定数 ===
#define FURNITURE_NUM (500)
#define FURNITURE_DETECTION_RANGE (5.0f) // Ghost検出範囲
#define FURNITURE_BILLBOARD_RANGE (3.0f) // ビルボード表示範囲の掛け率

// 暖炉の炎パラメータ（ここで一元管理）
#define CAMPFIRE_FIRE_SIZE_W   (2.0f)  // 炎の横サイズ
#define CAMPFIRE_FIRE_SIZE_H   (2.0f)  // 炎の縦サイズ
#define CAMPFIRE_FIRE_OFFSET_Y (0.5f)  // 暖炉基準の Y オフセット

//=== Busters 関連定数 ===
#define BUSTERS_MOVE_SPEED_SEARCH    (0.05f) // 探索速度
#define BUSTERS_MOVE_SPEED_SUSPICION (0.11f) // 警戒速度
#define BUSTERS_MOVE_SPEED_CHASE     (0.12f) // 追跡速度

#define MAP_MIN_X (-20.0f)
#define MAP_MAX_X (20.0f)
#define MAP_MIN_Z (-20.0f)
#define MAP_MAX_Z (20.0f)

#define BUSTERS_PATROL_RANGH (5.0f)      // 恐怖感知範囲
#define BUSTERS_SUSPICION_RANGE (10.0f)  // 怪しんで近づいてくる範囲
#define PATROL_HEIGHT (0.0f)
#define BUSTERS_HEIGHT (-0.5f)
#define BUSTERS_DEFOURT_GAUGE (50.0f)
#define WAIT_TIMER_DEFAULT (60)		     // 待機時間のデフォルト
#define WAIT_TIMER_COOLDOWN (1800)		 // 待機のクールタイム
#define KEEP_STATE_TIME (120)		 // 発見状態を維持する時間
#define BUSTERS_LURE_STAY_FRAMES (300)
#define BUSTERS_STOP_RANGE (5.0f)
#define BUSTERS_FOV_ANGLE (60.0f)  // 視野角（度）
#define BUSTERS_FOV_COS (0.5f)   

//=== Ghost 関連定数 ===
#define SCARE_RANGE (12.0f)		// 恐怖範囲
#define SCARE_COMBO_BASE_RADIUS (5.0f)
#define SCARE_COMBO_RADIUS_STEP (0.7f)
#define GHOST_MOVEMENT_SPEED (0.01f)
#define GHOST_ACCELERATION (0.010f)
#define GHOST_DECELERATION (0.98f)
#define GHOST_MAX_SPEED (0.10f)
#define FLOOR_COOLDOWN_TIME (2.0f) // 階層移動のクールダウン時間（秒）
#define GHOST_POS_Y (0.5f)

#define LURE_POSSESSED_SPEED_RATIO (0.5f)
#define GHOST_START_POS_FLOOR1_X (-1.0f)
#define GHOST_START_POS_FLOOR1_Z (-12.0f)
#define GHOST_START_POS_FLOOR2_X (-11.0f)
#define GHOST_START_POS_FLOOR2_Z (-11.0f)
#define GHOST_START_POS_FLOOR3_X (-4.0f)
#define GHOST_START_POS_FLOOR3_Z (4.0f)

//=== Minimap 関連定数 ===
#define MINIMAP_POS_OFFSET   (110.0f) 
#define BLOCK_SIZE      (7.0f)  
#define VIEW_RANGE      (13) 

//=== スコア関連定数 ===
#define SCARE_GAUGE_MAX (100.0f)// 恐怖ゲージの最大値
#define SCORE_LURE (0.5f) // 引き寄せスコア
#define SCORE_STOP (0.5f) // 停止スコア
#define SCORE_SCARE (3.9f) // 驚かせスコア//2.45f勝てなさすぎるので一時変更
#define BUSTER_GAUGE_REDUCTION (0.05f) // バスターに見つかったときのゲージ減少値（1フレームあたり）

//=== 家具耐性（慣れ）関連定数 ===
#define FURNITURE_RESISTANCE_DECAY  (0.85f)  // 同じ家具IDで使うたびに掛かる減衰率（1回目100%→2回目80%→...）
#define FURNITURE_RESISTANCE_MIN    (0.4f)  // 減衰の下限（最低でもこの倍率は保証する）

//=== UI_ScareCombo 関連定数 ===
#define SCARECOMBO_POS_X (SCREEN_WIDTH - 110.0f)
#define SCARECOMBO_POS_Y (170.0f)
#define SCARECOMBO_MAX (5)
#define SCARECOMBO_OVER_TIME (15000)// ミリ秒
#define SCARECOMBO_BAR_SIZE_X (140.0f)
#define SCARECOMBO_BAR_POS_X (SCARECOMBO_POS_X - 70.0f)