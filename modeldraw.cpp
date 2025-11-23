#include "modeldraw.h"
#include "Camera.h"
#include "newShader.h"

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

// モデル
static MODEL* g_testModel = nullptr;

void ModelDraw_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_testModel = ModelLoad("asset\\model\\ball.fbx");
}

void ModelDraw_Update(void)
{

}

void ModelDraw_DrawAll(void)
{
	// シェーダーを使ってパイプライン設定
	Shader_Begin();

	// プロジェクション・ビュー行列
	XMMATRIX View = GetCamera()->GetView();
	XMMATRIX Projection = GetCamera()->GetProjection();
	XMMATRIX VP = View * Projection;

	// スケーリング行列
	XMMATRIX ScalingMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);

	// 平行移動行列
	XMMATRIX TranslationMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);

	// 回転行列
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(0.0f),
		XMConvertToRadians(0.0f),
		XMConvertToRadians(0.0f));

	// モデル行列
	XMMATRIX Model_World = ScalingMatrix * RotationMatrix * TranslationMatrix;

	// 変換行列(WVP)
	XMMATRIX WVP = Model_World * VP; // (W * V * Projection)

	// シェーダーに変換行列をセット
	Shader_SetWorldMatrix(Model_World);
	Shader_SetMatrix(WVP);

	// モデル描画
	ModelDraw(g_testModel);
}

void ModelDraw_Finalize(void)
{
	ModelRelease(g_testModel);
}
