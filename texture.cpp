
#include "texture.h"
#include <Windows.h>
#include <cstdio>

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	TexMetadata metadata;
	ScratchImage image;
	ID3D11ShaderResourceView* g_Texture = nullptr;

	// 標準的な方法でロード（戻り値をチェック）
	HRESULT hr = LoadFromWICFile(texpass, WIC_FLAGS_FORCE_SRGB, &metadata, image);
	if (FAILED(hr))
	{
		wchar_t msg[512];
		swprintf_s(msg, L"LoadFromWICFile failed: hr=0x%08X path=%ls\n", static_cast<unsigned int>(hr), texpass ? texpass : L"(null)");
		OutputDebugStringW(msg);
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
		wchar_t msg[512];
		swprintf_s(msg, L"CreateShaderResourceView failed: hr=0x%08X path=%ls\n", static_cast<unsigned int>(hr), texpass ? texpass : L"(null)");
		OutputDebugStringW(msg);
		// 失敗時は NULL を返す（呼び出し側でフォールバック処理を行う）
		return nullptr;
	}

	return g_Texture;
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