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

#endif // GAME_H

