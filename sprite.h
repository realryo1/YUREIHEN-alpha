#pragma once

#include <d3d11.h>
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;

// 頂点構造体
struct Vertex
{
	XMFLOAT3 position; // 頂点座標  //XMFLOAT3へ変更
	XMFLOAT4 color;		//頂点カラー（R,G,B,A）
	XMFLOAT2 texCoord;	//テクスチャ座標
};

//プロトタイプ宣言
void InitializeSprite(void);	//スプライト初期化
void FinalizeSprite(void);	//スプライト終了
void Sprite_Draw(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* g_Texture);
