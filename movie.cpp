#include "movie.h"
#include "direct3d.h"
#include <mfapi.h>
#include <mfmediaengine.h>
#include <mfidl.h>
#include <d3d10.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

template<class T>
static void SafeReleaseCOM(T*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

class Movie::MediaEngineNotify : public IMFMediaEngineNotify
{
public:
    MediaEngineNotify() : m_refCount(1), m_canPlay(false), m_pEngine(nullptr) {}

    void SetEngine(IMFMediaEngine* pEngine)
    {
        m_pEngine = pEngine;
    }

    bool CanPlay() const
    {
        return m_canPlay;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == __uuidof(IMFMediaEngineNotify))
        {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return (ULONG)InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG count = (ULONG)InterlockedDecrement(&m_refCount);
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR, DWORD) override
    {
        if (meEvent == MF_MEDIA_ENGINE_EVENT_CANPLAY)
        {
            m_canPlay = true;
            if (m_pEngine)
            {
                m_pEngine->Play();
            }
        }
        return S_OK;
    }

private:
    ~MediaEngineNotify() = default;
    LONG m_refCount;
    bool m_canPlay;
    IMFMediaEngine* m_pEngine;
};

static std::wstring MakeFileUrl(const wchar_t* relativePath)
{
    wchar_t fullPath[MAX_PATH] = {};
    if (!GetFullPathNameW(relativePath, MAX_PATH, fullPath, nullptr))
    {
        return L"";
    }

    std::wstring url = L"file:///";

    for (wchar_t ch : std::wstring(fullPath))
    {
        url.push_back((ch == L'\\') ? L'/' : ch);
    }

    return url;
}

Movie::Movie(const XMFLOAT2& pos, const XMFLOAT2& size, float rotation, const XMFLOAT4& color, BLENDSTATE bstate, const wchar_t* filePath)
    : Transform2D(pos, rotation, size),
      m_Color(color),
      m_BlendState(bstate),
      m_pDXGIDeviceManager(nullptr),
      m_pMediaEngine(nullptr),
      m_pMediaEngineNotify(nullptr),
      m_pVideoTexture(nullptr),
      m_pVideoSRV(nullptr),
      m_DxgiResetToken(0)
{
    Initialize(filePath);
}

Movie::~Movie()
{
    Finalize();
}

bool Movie::Initialize(const wchar_t* filePath)
{
    ID3D11Device* pDevice = Direct3D_GetDevice();
    if (!pDevice) return false;
    Finalize();

    ID3D10Multithread* pMultithread = nullptr;
    HRESULT hr = pDevice->QueryInterface(__uuidof(ID3D10Multithread), (void**)&pMultithread);
    if (SUCCEEDED(hr) && pMultithread)
    {
        pMultithread->SetMultithreadProtected(TRUE);
        pMultithread->Release();
    }

    const UINT videoTexW = 1920;
    const UINT videoTexH = 1080;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = videoTexW;
    texDesc.Height = videoTexH;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    hr = pDevice->CreateTexture2D(&texDesc, nullptr, &m_pVideoTexture);
    if (FAILED(hr)) {
        Finalize();
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = pDevice->CreateShaderResourceView(m_pVideoTexture, &srvDesc, &m_pVideoSRV);
    if (FAILED(hr)) {
        Finalize();
        return false;
    }

    hr = MFCreateDXGIDeviceManager(&m_DxgiResetToken, &m_pDXGIDeviceManager);
    if (FAILED(hr)) {
        Finalize();
        return false;
    }

    hr = m_pDXGIDeviceManager->ResetDevice(pDevice, m_DxgiResetToken);
    if (FAILED(hr)) {
        Finalize();
        return false;
    }

    m_pMediaEngineNotify = new MediaEngineNotify();
    if (!m_pMediaEngineNotify) {
        Finalize();
        return false;
    }

    IMFAttributes* pAttributes = nullptr;
    IMFMediaEngineClassFactory* pFactory = nullptr;

    hr = MFCreateAttributes(&pAttributes, 4);
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, m_pDXGIDeviceManager);
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, m_pMediaEngineNotify);
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);
    }
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pFactory));
    }
    if (SUCCEEDED(hr))
    {
        hr = pFactory->CreateInstance(MF_MEDIA_ENGINE_REAL_TIME_MODE, pAttributes, &m_pMediaEngine);
    }

    SafeReleaseCOM(pFactory);
    SafeReleaseCOM(pAttributes);

    if (FAILED(hr) || !m_pMediaEngine)
    {
        Finalize();
        return false;
    }

    m_pMediaEngineNotify->SetEngine(m_pMediaEngine);
    m_pMediaEngine->SetLoop(TRUE);
    m_pMediaEngine->SetMuted(TRUE);

    std::wstring videoUrl = MakeFileUrl(filePath);
    if (videoUrl.empty()) {
        Finalize();
        return false;
    }

    BSTR bstrUrl = SysAllocString(videoUrl.c_str());
    if (!bstrUrl) {
        Finalize();
        return false;
    }

    m_pMediaEngine->SetSource(bstrUrl);
    SysFreeString(bstrUrl);
    m_pMediaEngine->Load();

    return true;
}

void Movie::Update()
{
    if (!m_pMediaEngine || !m_pVideoTexture) return;

    if (!m_pMediaEngine->HasVideo()) return;

    if (m_pMediaEngine->GetReadyState() < MF_MEDIA_ENGINE_READY_HAVE_CURRENT_DATA) return;

    LONGLONG pts = 0;
    HRESULT hr = m_pMediaEngine->OnVideoStreamTick(&pts);
    if (hr == S_OK)
    {
        D3D11_TEXTURE2D_DESC desc{};
        m_pVideoTexture->GetDesc(&desc);
        RECT dst = { 0, 0, (LONG)desc.Width, (LONG)desc.Height };
        MFARGB border = { 255, 0, 0, 0 };
        hr = m_pMediaEngine->TransferVideoFrame(m_pVideoTexture, nullptr, &dst, &border);
        if (FAILED(hr))
        {
            SafeReleaseCOM(m_pVideoSRV);
        }
    }
}

void Movie::Finalize()
{
    if (m_pMediaEngineNotify)
    {
        m_pMediaEngineNotify->SetEngine(nullptr);
    }

    if (m_pMediaEngine)
    {
        m_pMediaEngine->Shutdown();
    }

    SafeReleaseCOM(m_pMediaEngine);
    SafeReleaseCOM(m_pMediaEngineNotify);
    SafeReleaseCOM(m_pDXGIDeviceManager);
    SafeReleaseCOM(m_pVideoSRV);
    SafeReleaseCOM(m_pVideoTexture);
    m_DxgiResetToken = 0;
}

void Movie::Draw()
{
    if (m_pVideoSRV)
    {
        Sprite_Single_Draw(
            m_Position,
            m_Scale,
            m_Rotation,
            m_Color,
            m_BlendState,
            m_pVideoSRV
        );
    }
}

ID3D11ShaderResourceView* Movie::GetShaderResourceView() const
{
    return m_pVideoSRV;
}
