#pragma once

#include <d3d11.h>
#include <string>
#include "sprite.h"

struct IMFDXGIDeviceManager;
struct IMFMediaEngine;

class Movie : public Transform2D
{
public:
    Movie(const XMFLOAT2& pos, const XMFLOAT2& size, float rotation, const XMFLOAT4& color, BLENDSTATE bstate, const wchar_t* filePath);
    ~Movie();

    void Update();
    void Draw();

    ID3D11ShaderResourceView* GetShaderResourceView() const;

private:
    bool Initialize(const wchar_t* filePath);
    void Finalize();

    class MediaEngineNotify;

    XMFLOAT4 m_Color;
    BLENDSTATE m_BlendState;

    IMFDXGIDeviceManager* m_pDXGIDeviceManager;
    struct IMFMediaEngine* m_pMediaEngine;
    MediaEngineNotify* m_pMediaEngineNotify;
    ID3D11Texture2D* m_pVideoTexture;
    ID3D11ShaderResourceView* m_pVideoSRV;
    UINT m_DxgiResetToken;
};
