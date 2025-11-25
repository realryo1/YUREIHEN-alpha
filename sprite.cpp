
//sprite.cpp
#include "sprite.h"
#include "shader.h"
#include "main.h"

//グローバル変数
static constexpr int NUM_VERTEX = 6; // 使用できる最大頂点数
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


//----------------------------
//スプライト初期化
//----------------------------
void InitializeSprite()
{
	g_pDevice = Direct3D_GetDevice();

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;//<<<<<<<格納する最大頂点数
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

//----------------------------
//スプライト終了
//----------------------------
void FinalizeSprite()
{
	g_pVertexBuffer->Release();	//頂点バッファの解放
}

//----------------------------
//シンプルな四角形描画
//----------------------------
void Sprite_Draw(XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT4 color, BLENDSTATE bstate, ID3D11ShaderResourceView* g_Texture)
{
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点シェーダーに変換行列を設定
	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));


	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	//張るテクスチャの指定
	Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &g_Texture);
	SetBlendState(bstate);	//ブレンドを設定

	//頂点バッファのロック
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	//頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	v[0].position = { pos.x - (size.x / 2),pos.y - (size.y / 2), 0.0f };
	v[0].color = color;
	v[0].texCoord = { 0.0f,0.0f };

	v[1].position = { pos.x + (size.x / 2),pos.y - (size.y / 2), 0.0f };
	v[1].color = color;
	v[1].texCoord = { 1.0f,0.0f };

	v[2].position = { pos.x - (size.x / 2),pos.y + (size.y / 2), 0.0f };
	v[2].color = color;
	v[2].texCoord = { 0.0f,1.0f };

	v[3].position = { pos.x + (size.x / 2),pos.y + (size.y / 2), 0.0f };
	v[3].color = color;
	v[3].texCoord = { 1.0f,1.0f };

	//頂点バッファのロック解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//指定の位置に指定のサイズ、色の四角形を描画する
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	//ポリゴン描画方式の指定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0);
}
