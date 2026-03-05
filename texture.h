#pragma once
#pragma once
#include <d3d11.h>
#include <string>
#include "direct3d.h"
using namespace DirectX;
#include "debug_ostream.h"

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass);
ID3D11ShaderResourceView* LoadTexture(const std::wstring& texpass);
ID3D11ShaderResourceView* CreateRotated90Texture(ID3D11ShaderResourceView* srcSRV);
