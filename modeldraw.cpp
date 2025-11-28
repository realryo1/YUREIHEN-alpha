#include "modeldraw.h"
#include "Camera.h"
#include "shader.h"
#include "keyboard.h"

// グローバル変数
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;

// モデル
static MODEL* g_testModel = nullptr;

//回転
static float g_RotationY = 0.0f;

void ModelDraw_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_testModel = ModelLoad("asset\\model\\test.fbx");
}

void ModelDraw_Update(void)
{
	// スペースキーが押されたらy軸回転
	if (Keyboard_IsKeyDown(KK_I))
	{
		g_RotationY += 1.0f;
		if (g_RotationY >= 360.0f)
		{
			g_RotationY = 0.0f;
		}
	}
}

void ModelDraw_DrawAll(void)
{
	if (!g_testModel) return;

	// カメラ取得
	Camera* pCamera = GetCamera();
	if (!pCamera) return;

	// ビュー・プロジェクション行列の取得
	XMMATRIX View = pCamera->GetView();
	XMMATRIX Projection = pCamera->GetProjection();

	// モデルの変換行列
	XMMATRIX ScalingMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX TranslationMatrix = XMMatrixTranslation(1.0f, 2.0f, 1.0f);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(0.0f),
		XMConvertToRadians(g_RotationY),
		XMConvertToRadians(0.0f));

	// ワールド行列の合成（スケール → 回転 → 平行移動）
	XMMATRIX World = ScalingMatrix * RotationMatrix * TranslationMatrix;

	// WVP行列の計算
	XMMATRIX WVP = World * View * Projection;

	// シェーダーに行列をセット
	Shader_SetMatrix(WVP);           // WVP行列をセット
	Shader_SetWorldMatrix(World);    // ワールド行列をセット

	// シェーダーを使ってパイプライン設定
	Shader_Begin();

	// モデル描画
	ModelDraw(g_testModel);
}

void ModelDraw_Finalize(void)
{
	ModelRelease(g_testModel);
}