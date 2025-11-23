#include <directxmath.h>
#include "sprite.h"
#include "box.h"
using namespace DirectX;



static Vertex3D Box_vdata[BOX_NUM_VERTEX] =
{
	//---------------前面--------------------------------
	{//０　左上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//１　右上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//２　左下
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//５　右下
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,0,-1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//----------------------右側面------------------------
	{//６　左上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//７　右上
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//８　左下
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//１１　右下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------底面------------------------
	{//12　左上
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//13　右上
		XMFLOAT3(0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	{//14　左下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.75f)//テクスチャ座標
	},
	{//17　右下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,-1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.75f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------対面------------------------
	{//０　左上
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//１　右上
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//２　左下
		XMFLOAT3(0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//５　右下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(0,0,1),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//----------------------左側面------------------------
	{//６　左上
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//７　右上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	{//８　左下
		XMFLOAT3(-0.5f,-0.5f,0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.5f)//テクスチャ座標
	},
	{//１１　右下
		XMFLOAT3(-0.5f,-0.5f,-0.5f),//頂点座標
		XMFLOAT3(-1,0,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.5f)//テクスチャ座標
	},
	//----------------------------------------------------
	//------------------------天面------------------------
	{//12　左上
		XMFLOAT3(0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.0f)//テクスチャ座標
	},
	{//13　右上
		XMFLOAT3(-0.5f,0.5f,-0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.0f)//テクスチャ座標
	},
	{//14　左下
		XMFLOAT3(0.5f,0.5f,0.5f),//頂点座標
		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(0.0f,0.25f)//テクスチャ座標
	},
	{//17　右下
		XMFLOAT3(-0.5f,0.5f,0.5f),//頂点座標

		XMFLOAT3(0,1,0),//normal
		XMFLOAT4(1.0f,1.0f,1.0f,1.0f),//(R,G,B,A)
		XMFLOAT2(1.0f,0.25f)//テクスチャ座標
	},
	//----------------------------------------------------
};

//1ポリゴン=3頂点*2=1面＊6面
static UINT  Box_idxdata[6 * 6] =
{
	0,1,2,2,1,3,
	4,5,6,6,5,7,
	8,9,10,10,9,11,
	12,13,14,14,13,15,
	16,17,18,18,17,19,
	20,21,22,22,21,23
};

// BOXの一体を作成する
void CreateBox(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
	ID3D11Buffer** ppVertexBuffer, ID3D11Buffer** ppIndexBuffer)
{
	//頂点バッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Vertex3D) * BOX_NUM_VERTEX;//格納できる頂点数
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		pDevice->CreateBuffer(&bd, NULL, ppVertexBuffer);

		//頂点バッファをt頂点バッファに設定
		D3D11_MAPPED_SUBRESOURCE msr;
		pContext->Map(*ppVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		Vertex3D* vertex = (Vertex3D*)msr.pData;
		//頂点データコピー
		CopyMemory(&vertex[0], &Box_vdata[0], sizeof(Vertex3D) * BOX_NUM_VERTEX);
		//コピー完了
		pContext->Unmap(*ppVertexBuffer, 0);
	}

	//インデックスバッファ作成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(UINT) * 6 * 6;//格納できる頂点数
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		pDevice->CreateBuffer(&bd, NULL, ppIndexBuffer);

		//インデックスバッファに書き込み
		D3D11_MAPPED_SUBRESOURCE msr;
		pContext->Map(*ppIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		UINT* index = (UINT*)msr.pData;

		//インデックスデータコピー
		CopyMemory(&index[0], &Box_idxdata[0], sizeof(UINT) * 6 * 6);
		pContext->Unmap(*ppIndexBuffer, 0);
	}
}

