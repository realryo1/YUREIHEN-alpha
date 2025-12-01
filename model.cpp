#define NOMINMAX

#include "model.h"
#include "texture.h"
#include "shader.h"
#include "camera.h"
#include <DirectXMath.h>
#include <assert.h>
#include <iostream>
#include <float.h>
#include <algorithm>

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

// ノードを再帰的に描画する内部関数
void RenderNode(MODEL* model, aiNode* node, XMMATRIX parentTransform, const XMFLOAT4& color, bool useColorReplace = false)
{
	// このノードのローカル変換行列と親の変換を組み合わせ
	XMMATRIX currentTransform = AiMatrixToXMMatrix(node->mTransformation) * parentTransform;

	// このノード以下のすべてのメッシュを描画
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int meshIndex = node->mMeshes[i];
		aiMesh* mesh = model->AiScene->mMeshes[meshIndex];

		// マテリアルの色を計算
		XMFLOAT4 finalColor;
		if (useColorReplace)
		{
			// 色を置き換え（マテリアル色を無視）
			finalColor = color;
		}
		else
		{
			// ライト計算を有効化する処理
			// マテリアル色が黒い場合は白にリセット
			if (meshIndex < model->AiScene->mNumMeshes && model->MeshMaterials)
			{
				XMFLOAT4 meshColor = model->MeshMaterials[meshIndex].diffuseColor;
				
				// 【重要】メッシュの色が黒い場合は必ず白にリセット
				// これによりライトが正しく反映される
				if (meshColor.x == 0.0f && meshColor.y == 0.0f && meshColor.z == 0.0f)
				{
					meshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				}
				
				finalColor = XMFLOAT4(
					meshColor.x * color.x,
					meshColor.y * color.y,
					meshColor.z * color.z,
					meshColor.w * color.w
				);
			}
			else
			{
				finalColor = color;
			}
		}
		
		Shader_SetMaterialColor(finalColor);

		// テクスチャをシェーダーに設定(プリキャッシュされた値を使用)
		ID3D11ShaderResourceView* textureToSet = model->MeshMaterials[meshIndex].textureView;
		Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &textureToSet);

		// 頂点バッファ設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[meshIndex], &stride, &offset);

		// インデックスバッファ設定
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[meshIndex], DXGI_FORMAT_R32_UINT, 0);

		// 描画(保持されているインデックス数を使用)
		Direct3D_GetDeviceContext()->DrawIndexed(model->MeshIndexCounts[meshIndex], 0, 0);
	}

	// 子ノードを再帰実行
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		RenderNode(model, node->mChildren[i], currentTransform, color, useColorReplace);
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
	model->MeshIndexCounts = new unsigned int[model->AiScene->mNumMeshes];
	model->MeshMaterials = new MODEL::MeshMaterial[model->AiScene->mNumMeshes];

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// マテリアル情報の取得
		{
			aiMaterial* material = model->AiScene->mMaterials[mesh->mMaterialIndex];

			// ディフューズ色（基本色）を取得
			aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
			aiReturn colorResult = material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
			
			// マテリアル色が取得できなかった場合、または全て0の場合は白をデフォルトに設定
			if (colorResult != AI_SUCCESS || 
				(diffuseColor.r == 0.0f && diffuseColor.g == 0.0f && diffuseColor.b == 0.0f))
			{
				diffuseColor = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);  // デフォルトは白
				std::cout << "Mesh " << m << ": Material color not found or black, using white as default" << std::endl;
			}

			model->MeshMaterials[m].diffuseColor = XMFLOAT4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);

			// テクスチャ情報の取得
			aiString texturePath;
			if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
			{
				model->MeshMaterials[m].hasTexture = true;
				model->MeshMaterials[m].texturePath = texturePath.data;
			}
			else
			{
				model->MeshMaterials[m].hasTexture = false;
				model->MeshMaterials[m].texturePath.clear();
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
			model->MeshIndexCounts[m] = indexCount;

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

	// ダミー白テクスチャ（テクスチャなしメッシュ用）をモデルごとに読み込み
	model->WhiteTexture = LoadTexture(L"asset\\texture\\fade.png");
	if (!model->WhiteTexture)
	{
		std::cerr << "Failed to load white texture (fade.png)" << std::endl;
	}

	// メッシュごとのテクスチャをプリキャッシュ
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		if (model->MeshMaterials[m].hasTexture && model->Texture.count(model->MeshMaterials[m].texturePath))
		{
			model->MeshMaterials[m].textureView = model->Texture[model->MeshMaterials[m].texturePath];
		}
		else
		{
			model->MeshMaterials[m].textureView = model->WhiteTexture;
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
	delete[] model->MeshIndexCounts;
	delete[] model->MeshMaterials;

	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		if (pair.second)
			pair.second->Release();
	}

	if (model->WhiteTexture)
		model->WhiteTexture->Release();

	if (model->AiScene)
		aiReleaseImport(model->AiScene);

	delete model;
}

void ModelDraw(MODEL* model, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, const XMFLOAT4& color, bool useColorReplace)
{
	if (!model) return;

	// カメラ取得
	Camera* pCamera = GetCamera();
	if (!pCamera) return;

	// ビュー・プロジェクション行列の取得
	XMMATRIX View = pCamera->GetView();
	XMMATRIX Projection = pCamera->GetProjection();

	// モデルの変換行列
	XMMATRIX TranslationMatrix = XMMatrixTranslation(pos.x,pos.y,pos.z);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rot.x),
		XMConvertToRadians(rot.y),
		XMConvertToRadians(rot.z));
	XMMATRIX ScalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

	// ワールド行列の計算(スケール → 回転 → 移動の順)
	XMMATRIX World = ScalingMatrix * RotationMatrix * TranslationMatrix;

	// WVP行列の計算
	XMMATRIX WVP = World * View * Projection;

	// シェーダーに行列を設定
	Shader_SetMatrix(WVP);           // WVP行列を設定
	Shader_SetWorldMatrix(World);    // ワールド行列を設定

	// シェーダーを使用してパイプラインを設定
	Shader_Begin();

	// プリミティブ・トポロジーを設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ルートノードから再帰的に描画開始(変換はスケール行列)
	// colorが渡されなかった場合（デフォルト）は白色を確保
	XMFLOAT4 finalColor = color;
	
	// カラー変更を使用していないなら白に固定
	if (!useColorReplace)
	{
		finalColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
	XMMATRIX identity = XMMatrixIdentity();
	RenderNode(model, model->AiScene->mRootNode, identity, finalColor, useColorReplace);
}

XMFLOAT3 ModelGetSize(MODEL* model)
{
	if (!model || !model->AiScene || model->AiScene->mNumMeshes == 0)
	{
		return XMFLOAT3(1.0f, 1.0f, 1.0f);  // デフォルトサイズを返す
	}

	// バウンディングボックスの最小・最大座標を計算
	float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

	// すべてのメッシュから頂点を取得してバウンディングボックスを計算
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			const aiVector3D& vertex = mesh->mVertices[v];

			minX = std::min(minX, vertex.x);
			minY = std::min(minY, vertex.y);
			minZ = std::min(minZ, vertex.z);

			maxX = std::max(maxX, vertex.x);
			maxY = std::max(maxY, vertex.y);
			maxZ = std::max(maxZ, vertex.z);
		}
	}

	// 無限大のままの場合はデフォルト値を返す
	if (minX == FLT_MAX || maxX == -FLT_MAX)
	{
		return XMFLOAT3(1.0f, 1.0f, 1.0f);
	}

	// サイズを計算（最大座標 - 最小座標）
	XMFLOAT3 size(
		maxX - minX,
		maxY - minY,
		maxZ - minZ
	);

	// サイズが0の場合は最小値を設定
	if (size.x == 0.0f) size.x = 1.0f;
	if (size.y == 0.0f) size.y = 1.0f;
	if (size.z == 0.0f) size.z = 1.0f;

	return size;
}

// モデルの平均的なマテリアルカラーを取得する関数
XMFLOAT4 ModelGetAverageMaterialColor(MODEL* model)
{
	if (!model || !model->AiScene || model->AiScene->mNumMeshes == 0)
	{
		return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // デフォルトは白
	}

	float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

	// すべてのメッシュのマテリアルカラーの平均を計算
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		r += model->MeshMaterials[m].diffuseColor.x;
		g += model->MeshMaterials[m].diffuseColor.y;
		b += model->MeshMaterials[m].diffuseColor.z;
		a += model->MeshMaterials[m].diffuseColor.w;
	}

	unsigned int meshCount = model->AiScene->mNumMeshes;
	return XMFLOAT4(
		r / meshCount,
		g / meshCount,
		b / meshCount,
		a / meshCount
	);
}
