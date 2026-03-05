#include "texture.h"
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	TexMetadata metadata;
	ScratchImage image;
	ID3D11ShaderResourceView* g_Texture = nullptr;

	// 標準的な方法でロード（戻り値をチェック）
	HRESULT hr = LoadFromWICFile(texpass, WIC_FLAGS_FORCE_SRGB, &metadata, image);
	if (FAILED(hr))
	{
		return nullptr;
	}

	// 標準的に SRV を作成（戻り値をチェック）
	hr = CreateShaderResourceView(
		Direct3D_GetDevice(),
		image.GetImages(),
		image.GetImageCount(),
		metadata,
		&g_Texture
	);

	if (FAILED(hr) || g_Texture == nullptr)
	{
		// 失敗時は NULL を返す（呼び出し側でフォールバック処理を行う）
		return nullptr;
	}

	std::string texpassStr;
	size_t len = wcslen(texpass);
	for (size_t i = 0; i < len; ++i) texpassStr += static_cast<char>(texpass[i]);
	hal::dout << texpassStr << std::endl;
	
	return g_Texture;
}

ID3D11ShaderResourceView* LoadTexture(const std::wstring& texpass)
{
	return LoadTexture(texpass.c_str());
}

// SRVから元テクスチャのピクセルを読み取り、90°時計回りに回転した新しいSRVを生成する
ID3D11ShaderResourceView* CreateRotated90Texture(ID3D11ShaderResourceView* srcSRV)
{
	if (srcSRV == nullptr) return nullptr;

	// SRVから元のTexture2Dを取得
	ID3D11Resource* pRes = nullptr;
	srcSRV->GetResource(&pRes);
	if (!pRes) return nullptr;

	ID3D11Texture2D* pSrcTex = nullptr;
	HRESULT hr = pRes->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pSrcTex);
	pRes->Release();
	if (FAILED(hr) || !pSrcTex) return nullptr;

	D3D11_TEXTURE2D_DESC srcDesc;
	pSrcTex->GetDesc(&srcDesc);

	UINT srcW = srcDesc.Width;
	UINT srcH = srcDesc.Height;

	// ステージングテクスチャを作成してGPUからピクセルを読み取る
	D3D11_TEXTURE2D_DESC stagingDesc = srcDesc;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;
	stagingDesc.MipLevels = 1;
	stagingDesc.ArraySize = 1;

	ID3D11Device* pDevice = Direct3D_GetDevice();
	ID3D11DeviceContext* pContext = Direct3D_GetDeviceContext();

	ID3D11Texture2D* pStaging = nullptr;
	hr = pDevice->CreateTexture2D(&stagingDesc, nullptr, &pStaging);
	if (FAILED(hr)) { pSrcTex->Release(); return nullptr; }

	pContext->CopyResource(pStaging, pSrcTex);
	pSrcTex->Release();

	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = pContext->Map(pStaging, 0, D3D11_MAP_READ, 0, &mapped);
	if (FAILED(hr)) { pStaging->Release(); return nullptr; }

	// 90°時計回り回転: dst(x,y) = src(y, srcW-1-x)
	// 回転後のサイズ: dstW=srcH, dstH=srcW
	UINT dstW = srcH;
	UINT dstH = srcW;
	UINT bpp = 4; // RGBA 4バイトと仮定
	std::vector<uint8_t> dstPixels(dstW * dstH * bpp);

	const uint8_t* srcData = (const uint8_t*)mapped.pData;
	for (UINT sy = 0; sy < srcH; sy++)
	{
		for (UINT sx = 0; sx < srcW; sx++)
		{
			UINT dx = sy;
			UINT dy = srcW - 1 - sx;
			const uint8_t* srcPixel = srcData + sy * mapped.RowPitch + sx * bpp;
			uint8_t* dstPixel = &dstPixels[(dy * dstW + dx) * bpp];
			dstPixel[0] = srcPixel[0];
			dstPixel[1] = srcPixel[1];
			dstPixel[2] = srcPixel[2];
			dstPixel[3] = srcPixel[3];
		}
	}

	pContext->Unmap(pStaging, 0);
	pStaging->Release();

	// 回転後のテクスチャを作成
	D3D11_TEXTURE2D_DESC dstDesc = srcDesc;
	dstDesc.Width = dstW;
	dstDesc.Height = dstH;
	dstDesc.MipLevels = 1;
	dstDesc.ArraySize = 1;
	dstDesc.Usage = D3D11_USAGE_DEFAULT;
	dstDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dstDesc.CPUAccessFlags = 0;
	dstDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = dstPixels.data();
	initData.SysMemPitch = dstW * bpp;
	initData.SysMemSlicePitch = 0;

	ID3D11Texture2D* pDstTex = nullptr;
	hr = pDevice->CreateTexture2D(&dstDesc, &initData, &pDstTex);
	if (FAILED(hr)) return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = dstDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	ID3D11ShaderResourceView* pDstSRV = nullptr;
	hr = pDevice->CreateShaderResourceView(pDstTex, &srvDesc, &pDstSRV);
	pDstTex->Release();

	if (FAILED(hr)) return nullptr;
	return pDstSRV;
}


//#include "texture.h"
//
//ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
//{
//	TexMetadata metadata;
//	ScratchImage image; 
//	ID3D11ShaderResourceView* g_Texture = NULL;
//
//	// 標準的な方法でロード
//	// WIC_FLAGS_NONE: メタデータをそのまま使用
//	LoadFromWICFile(texpass, WIC_FLAGS_FORCE_SRGB, &metadata, image);
//	// sRGB変換しない
//	
//	//// メタデータでsRGB対応フォーマットをチェック
//	//bool isSRGB = DirectX::IsSRGB(metadata.format);
//	//
//	//// sRGB対応フォーマットでない場合は変換
//	//if (!isSRGB)
//	//{
//	//	// 線形フォーマット → sRGB対応フォーマットに変換
//	//	metadata.format = DirectX::MakeSRGB(metadata.format);
//	//}
//	//
//	//// フォーマットをオーバーライド
//	//image.OverrideFormat(metadata.format);
//	
//	// 標準的に作成
//	CreateShaderResourceView(
//		Direct3D_GetDevice(),
//		image.GetImages(),
//		image.GetImageCount(),
//		metadata,
//		&g_Texture
//	);
//	
//	assert(g_Texture);		//ロード失敗時にダイアログを表示
//
//	return g_Texture;
//}
