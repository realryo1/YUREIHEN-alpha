/*==============================================================================

   ポリゴン描画 [game.h]
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef GAME_H
#define GAME_H

#include <d3d11.h>

enum MOVE
{
	STOP = 0,
	RIGHT,
	LEFT
};

void Game_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Game_Finalize(void);
void Game_Update(void);
void Game_Draw(void);

// チュートリアル用：円盤接触フラグのポインタを返す
bool* Game_GetEnbanTouchedPtr(void);

// フロア降下アニメーション中かどうか
bool  Game_IsFloorExitAnimActive(void);

// 俯瞰カメラ・補間中かどうか（OVERVIEW or CAM_LERP）
// PLAYER_WALKはカメラ追従を再開するためfalseを返す
bool  Game_IsCamOverrideActive(void);

// 敗北アニメーション中かどうか
bool  Game_IsLoseAnimActive(void);

// 敗北アニメーションを開始する
void  Game_StartLoseAnim(void);

// 敗北アニメーションを終了しリソースを解放する（リトライメニューから呼ぶ）
void  Game_EndLoseAnim(void);

// フロアを強制的にスキップし次の階へ進む（ポーズメニュー用）
void  Game_ForceSkipFloor(void);

#endif // GAME_H
