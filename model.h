#pragma once
//#define NOMINMAX
#include <unordered_map>
#include <vector>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include	"d3d11.h"
#include	"DirectXMath.h"
using namespace DirectX;
#include	"direct3d.h"


// ボーン行列情報構造体
struct BoneMatrices
{
	static const unsigned int MAX_BONES = 256;  // 最大ボーン数
	XMMATRIX matrices[MAX_BONES];               // ボーン行列配列
	unsigned int boneCount = 0;                 // 実際のボーン数

	BoneMatrices()
	{
		for (unsigned int i = 0; i < MAX_BONES; i++)
		{
			matrices[i] = XMMatrixIdentity();
		}
		boneCount = 0;
	}
};

struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer;
	ID3D11Buffer** IndexBuffer;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	// メッシュ単位のインデックス数
	unsigned int* MeshIndexCounts;
	
	// メッシュ単位のマテリアル情報
	struct MeshMaterial
	{
		XMFLOAT4 diffuseColor;
		bool hasTexture;
		std::string texturePath;
		ID3D11ShaderResourceView* textureView;
	}* MeshMaterials;

	// 白テクスチャ（テクスチャ無しメッシュ用）
	ID3D11ShaderResourceView* WhiteTexture;
};

// Assimpの行列をDirectXMath形式に変換（外部で利用可能）
XMMATRIX AiMatrixToXMMatrix(const aiMatrix4x4& mat);

MODEL* ModelLoad(const char* FileName);
void ModelRelease(MODEL* model);
void ModelDraw(MODEL* model, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), bool useColorReplace = false);

// アニメーション対応の描画関数
void ModelAnimationDraw(MODEL* model, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, const BoneMatrices& boneMatrices, const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), bool useColorReplace = false);

// アニメーション計算関数
void ModelCalculateBoneMatrices(MODEL* model, double animationTime, BoneMatrices& outBoneMatrices);

XMFLOAT3 ModelGetSize(MODEL* model);
XMFLOAT4 ModelGetAverageMaterialColor(MODEL* model);