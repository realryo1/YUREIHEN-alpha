#include "texture.h"

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	TexMetadata metadata;
	ScratchImage image; 
	ID3D11ShaderResourceView* g_Texture = NULL;

	LoadFromWICFile(texpass, WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(Direct3D_GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);		//読み込み失敗時にダイアログを表示

	return g_Texture;
}
