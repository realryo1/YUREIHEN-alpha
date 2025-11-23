/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H


#include <Windows.h>
#include <d3d11.h>
#include <mmsystem.h>

#include "DirectXTex.h"/////////////0602
#if _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex_Release.lib");
#endif
// セーフリリースマクロ
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }


bool Direct3D_Initialize(HWND hWnd); // Direct3Dの初期化
void Direct3D_Finalize(); // Direct3Dの終了処理

void Direct3D_Clear(); // バックバッファのクリア
void Direct3D_Present(); // バックバッファの表示

////////////////////////////////////////////////追加
ID3D11Device* Direct3D_GetDevice(); // デバイスの取得
ID3D11DeviceContext* Direct3D_GetDeviceContext(); // デバイスコンテキストの取得

unsigned int Direct3D_GetBackBufferWidth(); // バックバッファの幅を取得
unsigned int Direct3D_GetBackBufferHeight(); // バックバッファの高さを取得


void	SetDepthTest(bool flg);	//深度テスト切り替え


enum	BLENDSTATE
{
	BLENDSTATE_NONE = 0,	//ブレンドしない
	BLENDSTATE_ALFA,		//普通のαブレンド
	BLENDSTATE_ADD,			//加算合成 
	BLENDSTATE_SUB,			//減算合成

	BLENDSTATE_MAX
};
void SetBlendState(BLENDSTATE blend);


//ブロック縦横配列サイズ
#define BLOCK_COLS		(6)	//ブロックスタック横の数
#define BLOCK_ROWS		(13)//ブロックスタック縦の数

//ブロックサイズ
#define		BLOCK_WIDTH		(50.0f)
#define		BLOCK_HEIGHT	(50.0f)

//スクロール値
#define		POSITION_OFFSET_X	(490.0f)
#define		POSITION_OFFSET_Y	(34.0f)


////////////////////////////////////////////////////


using namespace DirectX;
struct Vertex3D
{
	XMFLOAT3 position; // 頂点座標  //XMFLOAT3へ変更
	XMFLOAT3 normal;
	XMFLOAT4 color;		//頂点カラー（R,G,B,A）
	XMFLOAT2 texCoord;	//テクスチャ座標
};

class Light
{
protected:
	BOOL enable;	//ライトの有効無効
	BOOL dummy[3];
	XMFLOAT4 direction;	//ライトの向き（正規化する必要がある）
	XMFLOAT4 diffuse;	//光の色
	XMFLOAT4 ambient;	//環境光
public:
	Light(BOOL e, XMFLOAT4 direction, XMFLOAT4 diffuse, XMFLOAT4 ambient)
		: enable(e),dummy(), diffuse(diffuse), ambient(ambient) {
		static float len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
		direction.x /= len;
		direction.y /= len;
		direction.z /= len;
		this->direction = direction;
	}

	void SetEnable(BOOL enable) { this->enable = enable; }
};

#endif // DIRECT3D_H

