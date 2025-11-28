#define NOMINMAX

#include "model.h"
#include "texture.h"
#include "shader.h"
#include <DirectXMath.h>
#include <assert.h>
#include <iostream>

using namespace DirectX;

// Assimpの行列をDirectXMath形式に変換
XMMATRIX AiMatrixToXMMatrix(const aiMatrix4x4& mat)
{
	return XMMATRIX(
		mat.a1, mat.a2, mat.a3, mat.a4,
		mat.b1, mat.b2, mat.b3, mat.b4,
		mat.c1, mat.c2, mat.c3, mat.c4,
		mat.d1, mat.d2, mat.d3, mat.d4
	);
}

// メッシュの情報を保持する構造体
struct MeshData
{
	unsigned int indexCount;  // インデックス数
};

// グローバル変数（メッシュごとのインデックス数を保持）
static MeshData* g_meshData = nullptr;
static unsigned int g_meshCount = 0;

// メッシュマテリアル情報構造体
struct MeshMaterialData
{
	XMFLOAT4 diffuseColor;     // ディフューズ色（ハイパーシェード設定）
	bool hasTexture;           // テクスチャを持つか
	std::string texturePath;   // テクスチャパス
};

// グローバル変数（メッシュごとのマテリアル情報）
static MeshMaterialData* g_meshMaterialData = nullptr;

// ダミーテクスチャ（テクスチャなしメッシュ用）
static ID3D11ShaderResourceView* g_pWhiteTexture = nullptr;

// ノードを再帰的に描画する内部関数
void RenderNode(MODEL* model, aiNode* node, XMMATRIX parentTransform)
{
	// このノードのローカル変換行列を親の変換と合成
	XMMATRIX currentTransform = AiMatrixToXMMatrix(node->mTransformation) * parentTransform;

	// このノードが持つすべてのメッシュを描画
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int meshIndex = node->mMeshes[i];
		aiMesh* mesh = model->AiScene->mMeshes[meshIndex];

		// マテリアル色をシェーダーに設定
		if (meshIndex < g_meshCount)
		{
			Shader_SetMaterialColor(g_meshMaterialData[meshIndex].diffuseColor);
		}

		// テクスチャ設定：前回のテクスチャ残存を防ぐため毎回セット
		aiString texturePath;
		aiMaterial* material = model->AiScene->mMaterials[mesh->mMaterialIndex];
		material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);

		ID3D11ShaderResourceView* textureToSet = g_pWhiteTexture;  // デフォルトは白テクスチャ
		if (texturePath.length > 0 && model->Texture.count(texturePath.data))
		{
			textureToSet = model->Texture[texturePath.data];
		}
		// テクスチャがない場合は白テクスチャを使用
		Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &textureToSet);

		// 頂点バッファ設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[meshIndex], &stride, &offset);

		// インデックスバッファ設定
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[meshIndex], DXGI_FORMAT_R32_UINT, 0);

		// 描画（保持されているインデックス数を使用）
		Direct3D_GetDeviceContext()->DrawIndexed(g_meshData[meshIndex].indexCount, 0, 0);
	}

	// 子ノードを再帰処理
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		RenderNode(model, node->mChildren[i], currentTransform);
	}
}

MODEL* ModelLoad(const char* FileName)
{
	MODEL* model = new MODEL;

	const std::string modelPath(FileName);

	// Assimpのフラグを改善: Triangulateフラグで自動的に三角形化
	model->AiScene = aiImportFile(FileName, 
		aiProcessPreset_TargetRealtime_MaxQuality | 
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |              // 四角形以上を三角形化
		aiProcess_GenSmoothNormals |         // スムーズ法線生成
		aiProcess_JoinIdenticalVertices |    // 重複頂点削除
		aiProcess_OptimizeGraph              // グラフ最適化
	);
	assert(model->AiScene);

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];

	// メッシュデータ初期化
	g_meshCount = model->AiScene->mNumMeshes;
	g_meshData = new MeshData[g_meshCount];
	g_meshMaterialData = new MeshMaterialData[g_meshCount];

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// マテリアル情報取得
		{
			aiMaterial* material = model->AiScene->mMaterials[mesh->mMaterialIndex];

			// ディフューズ色（基本色）を取得
			aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
			if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor))
			{
				diffuseColor = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);  // デフォルトは白
			}

			g_meshMaterialData[m].diffuseColor = XMFLOAT4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);

			// テクスチャ情報を取得
			aiString texturePath;
			if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
			{
				g_meshMaterialData[m].hasTexture = true;
				g_meshMaterialData[m].texturePath = texturePath.data;
			}
			else
			{
				g_meshMaterialData[m].hasTexture = false;
				g_meshMaterialData[m].texturePath.clear();
			}
		}

		// 頂点バッファ生成
		{
			Vertex3D* vertex = new Vertex3D[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				// 座標変換注意: aiProcess_ConvertToLeftHandedを使う場合は素直に代入
				vertex[v].position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
				
				// テクスチャ座標が存在するかチェック
				if (mesh->HasTextureCoords(0))
				{
					vertex[v].texCoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
				}
				else
				{
					// テクスチャ座標がない場合はデフォルト値
					vertex[v].texCoord = XMFLOAT2(0.5f, 0.5f);
				}
				
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				
				// 法線が存在するかチェック
				if (mesh->HasNormals())
				{
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}
				else
				{
					// 法線がない場合はデフォルト（上向き）
					vertex[v].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(Vertex3D) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);
			if (FAILED(hr))
			{
				std::cerr << "Failed to create vertex buffer for mesh " << m << std::endl;
				delete[] vertex;
				return nullptr;
			}

			delete[] vertex;

			// デバッグ情報出力
			std::cout << "Mesh " << m << " created: " 
					  << mesh->mNumVertices << " vertices, "
					  << (mesh->HasNormals() ? "WITH" : "WITHOUT") << " normals, "
					  << (mesh->HasTextureCoords(0) ? "WITH" : "WITHOUT") << " UVs" << std::endl;
		}

		// インデックスバッファ生成
		{
			// 三角形化されているため、すべてのフェイスは3つのインデックスを持つ
			unsigned int indexCount = 0;
			
			// インデックス数を計算
			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];
				
				if (face->mNumIndices >= 3)
				{
					// 三角形化後は通常3、稀に4以上の場合は最初の三角形のみを使用
					indexCount += 3;
				}
			}

			// インデックス数を保存
			g_meshData[m].indexCount = indexCount;

			std::cout << "Mesh " << m << ": " << indexCount << " indices (" << mesh->mNumFaces << " faces)" << std::endl;

			unsigned int* index = new unsigned int[indexCount];
			unsigned int indexOffset = 0;

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				// 三角形チェック（より柔軟に対応）
				if (face->mNumIndices >= 3 && indexOffset + 3 <= indexCount)
				{
					index[indexOffset + 0] = face->mIndices[0];
					index[indexOffset + 1] = face->mIndices[1];
					index[indexOffset + 2] = face->mIndices[2];
					indexOffset += 3;
				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * indexCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
			if (FAILED(hr))
			{
				std::cerr << "Failed to create index buffer for mesh " << m << std::endl;
				delete[] index;
				return nullptr;
			}

			delete[] index;
		}
	}

	// テクスチャ読み込み
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture;
		TexMetadata metadata;
		ScratchImage image;
		
		HRESULT hr = LoadFromWICMemory(aitexture->pcData, aitexture->mWidth, WIC_FLAGS_NONE, &metadata, image);
		if (FAILED(hr))
		{
			std::cerr << "Failed to load texture from memory: " << aitexture->mFilename.data << std::endl;
			continue;
		}
		
		hr = CreateShaderResourceView(Direct3D_GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
		if (FAILED(hr))
		{
			std::cerr << "Failed to create shader resource view for texture: " << aitexture->mFilename.data << std::endl;
			continue;
		}

		model->Texture[aitexture->mFilename.data] = texture;
	}

	// ダミー白テクスチャ（テクスチャなしメッシュ用）をロード
	if (!g_pWhiteTexture)
	{
		g_pWhiteTexture = LoadTexture(L"asset\\texture\\fade.png");
		if (!g_pWhiteTexture)
		{
			std::cerr << "Failed to load white texture (fade.png)" << std::endl;
		}
	}

	return model;
}

void ModelRelease(MODEL* model)
{
	if (!model) return;

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		if (model->VertexBuffer[m])
			model->VertexBuffer[m]->Release();
		if (model->IndexBuffer[m])
			model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;

	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		if (pair.second)
			pair.second->Release();
	}

	if (model->AiScene)
		aiReleaseImport(model->AiScene);

	if (g_meshData)
		delete[] g_meshData;
	
	if (g_meshMaterialData)
		delete[] g_meshMaterialData;

	delete model;
}

void ModelDraw(MODEL* model)
{
	if (!model) return;

	// プリミティブトポロジ設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ルートノードから再帰的に描画開始（初期変換は単位行列）
	XMMATRIX identity = XMMatrixIdentity();
	RenderNode(model, model->AiScene->mRootNode, identity);
}

// オプション: モデル全体のTransformを分解して取得する関数
void ModelGetTransform(MODEL* model, XMFLOAT3& outPosition, XMFLOAT4& outRotation, XMFLOAT3& outScale)
{
	if (!model || !model->AiScene) return;

	aiVector3D scaling, position;
	aiQuaternion rotation;

	model->AiScene->mRootNode->mTransformation.Decompose(scaling, rotation, position);

	outPosition = XMFLOAT3(position.x, position.y, position.z);
	outRotation = XMFLOAT4(rotation.x, rotation.y, rotation.z, rotation.w);
	outScale = XMFLOAT3(scaling.x, scaling.y, scaling.z);
}
