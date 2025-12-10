#ifndef NOMINMAX
#define NOMINMAX
#endif
// Logo.cpp
#include "Fade.h"
#include "shader.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "direct3d.h"
#include "texture.h"
#include "scene.h"
#include <DirectXMath.h>
#include <cmath>
#include <d3d11.h>
#include <cassert>
#include <cstdint>

using namespace DirectX;

static ID3D11ShaderResourceView* g_Texture[4];
static ID3D11ShaderResourceView* g_SolidTex = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static bool fadeStarted = false;
static float alpha[3] = { 1.0f, 0.0f, 0.0f };

// 幽霊
static XMFLOAT2 g_ghostOffset = { 0.0f, 0.0f };
static float g_ghostBobRotation = 0.0f;
static float g_ghostAngle = 0.0f;
static float g_ghostTargetAngle = 0.0f;
static bool g_ghostFacingLeft = false;
static bool g_prevGhostFacingLeft = false;
static float g_ghostScale = 1.0f;
static bool g_forceFacingByTimer = true;

// 右を向いたときに一度だけ縮小アニメ（幽霊）
static bool g_shrinkTriggered = false;
static bool g_shrinkAppliedOnce = false;
static float g_shrinkTimer = 0.0f;
static const float g_shrinkDuration = 0.45f;
static const float g_shrinkTargetScale = 0.45f;

// basuta
static XMFLOAT2 g_basutaOffset = { 0.0f, 0.0f };
static float g_basutaScale = 1.0f;                     // 追加: basuta 用スケール
static const float g_basutaTargetScale = 0.60f;        // basuta 縮小後スケール
static const float g_basutaShrinkStart = 500.0f;       // 近づきはじめる距離（調整可）

// bikkuri（ビックリマーク）表示管理
static bool g_bikkuriShown = false;       // 現在表示中か
static float g_bikkuriTimer = 0.0f;
static const float g_bikkuriDuration = 0.9f;
static bool g_bikkuriFlip = false;        // 反転フラグ（描画時使用）
static const float g_bikkuriLeadTime = 0.35f;
static bool g_bikkuriShownOnce = false;   // 描画確定後に true になる（再表示防止）

// inazuma（稲妻）管理（右端に表示、ランダム発生・フラッシュ演出）
static float g_inazumaTimer = 0.0f;
static float g_inazumaNextStrike = 3.0f;
static float g_inazumaStrikeDuration = 0.15f;
static bool g_inazumaActive = false;
static float g_inazumaFlash = 0.0f; // 画面全体フラッシュ用（0..1）
static unsigned int g_inazumaSeed = 0xC0FFEEu;
static float g_inazumaBoltAlphas[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static Sprite* g_inazumaSprite = nullptr; // 稲妻用 Sprite（basuta が上書きされないように追加）

// 設定・状態
static const float g_ghostLeadSeconds = 7.0f;
static const float g_basutaSpeed = 220.0f;
enum GhostState { GHOST_IDLE = 0, GHOST_ALERT, GHOST_MOVE_TO_HOUSE };
static GhostState g_ghostState = GHOST_IDLE;

static const XMFLOAT2 g_imageSize = { 500.0f, 500.0f };

// 位置
static XMFLOAT2 g_yakataPos = { 0.0f, 0.0f };
static XMFLOAT2 g_ghostPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaPos = { 0.0f, 0.0f };
static XMFLOAT2 g_basutaTarget = { 0.0f, 0.0f };
static bool g_positionsInitialized = false;
static bool g_basutaMoving = false;
static bool g_basutaStartFromRight = false;
static bool g_basutaEnteredScreen = false;

static const float PI = 3.14159265358979323846f;

// Sprite インスタンスを管理するポインタ
static Sprite* g_yakataSprite = nullptr;
static Sprite* g_ghostSprite = nullptr;
static Sprite* g_basutaSprite = nullptr;


static float AngleDelta(float target, float current)
{
    float diff = target - current;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    return diff;
}

static float EaseOutCubic(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

// ランダム(0..1)
static float Rand01()
{
    g_inazumaSeed = g_inazumaSeed * 1664525u + 1013904223u;
    return (float)(g_inazumaSeed & 0x00FFFFFFu) / (float)0x01000000u;
}

// ヘルパー: 単色 1x1 テクスチャ SRV を作る（フォールバック用）
static ID3D11ShaderResourceView* CreateSolidSRV(ID3D11Device* device, uint32_t rgba)
{
    if (!device) return nullptr;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = &rgba;
    sd.SysMemPitch = sizeof(rgba);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &sd, &tex);
    if (FAILED(hr) || !tex) return nullptr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
    tex->Release();
    if (FAILED(hr)) { if (srv) srv->Release(); return nullptr; }
    return srv;
}

void OpAnim_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    SetFPS(30);

    // デバイス / コンテキストを保存（描画時に使用）
    g_pDevice = pDevice;
    g_pContext = pContext;

    // Sprite インスタンスの作成
    // 屋敷スプライト
    g_yakataSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置
        g_imageSize,                      // サイズ
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 色
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\yakata_jimen1.png"  // テクスチャパス
    );

    // 幽霊スプライト
    g_ghostSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置
        g_imageSize,                      // サイズ
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), // 色（初期状態では透明）
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\yurei1.png"    // テクスチャパス
    );

    // basuta スプライト
    g_basutaSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置
        g_imageSize,                      // サイズ
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), // 色（初期状態では透明）
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\basuta1.png"   // テクスチャパス
    );

    // inazuma スプライト（修正: basuta を上書きしていた箇所を修正して別インスタンスにする）
    g_inazumaSprite = new Sprite(
        XMFLOAT2(0.0f, 0.0f),           // 初期位置（描画時に更新）
        XMFLOAT2(300.0f, 720.0f),         // 稲妻は縦長にしておく
        0.0f,                             // 回転
        XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), // 色（初期状態では透明）
        BLENDSTATE_ALFA,                  // ブレンドステート
        L"asset\\yureihen\\inazuma.png"   // テクスチャパス
    );

    // Sprite 側とは別に、ここで描画に使用する SRV をロードしておく
    for (int i = 0; i < 5; ++i) g_Texture[i] = nullptr;
    g_Texture[0] = LoadTexture(L"asset\\yureihen\\yakata_jimen1.png");
    g_Texture[1] = LoadTexture(L"asset\\yureihen\\yurei1.png");
    g_Texture[2] = LoadTexture(L"asset\\yureihen\\basuta1.png");
    g_Texture[3] = LoadTexture(L"asset\\yureihen\\bikkuri.png"); // bikkuri のファイル名に合わせる
    g_Texture[4] = LoadTexture(L"asset\\yureihen\\inazuma.png"); // 稲妻テクスチャ

    // LoadTexture が失敗した場合は目印になる 1x1 テクスチャで置き換える（NULL 回避）
    for (int i = 0; i < 4; ++i)
    {
        if (!g_Texture[i] && g_pDevice)
        {
            // マゼンタ (アルファ 1) を目印にする：0xFF00FF00 ARGB? ここは RGBA リトルエンディアンで指定
            // R=255,G=0,B=255,A=255 -> 0xFF00FF00 は ARGB の可能性が混乱するので明示的に RGBA 値を作る:
            uint32_t r = 255u, g = 0u, b = 255u, a = 255u;
            uint32_t rgba = (r) | (g << 8) | (b << 16) | (a << 24);
            g_Texture[i] = CreateSolidSRV(g_pDevice, rgba);
        }
    }

    // 1x1 のソリッドテクスチャ（白）を作成して g_SolidTex にセット（背景等用）
    if (g_pDevice && !g_SolidTex)
    {
        uint32_t white = (255u) | (255u << 8) | (255u << 16) | (255u << 24);
        g_SolidTex = CreateSolidSRV(g_pDevice, white);
    }

    alpha[0] = 1.0f;
    alpha[1] = 0.0f;
    alpha[2] = 0.0f;

    g_ghostAngle = 0.0f;
    g_ghostTargetAngle = 0.0f;
    g_ghostState = GHOST_IDLE;
    g_ghostFacingLeft = false;
    g_ghostScale = 1.0f;

    // 稲妻初期値
    g_inazumaTimer = 0.0f;
    g_inazumaNextStrike = 2.0f + Rand01() * 4.0f; // 2..6 秒
    g_inazumaActive = false;
    g_inazumaFlash = 0.0f;
    for (int i = 0; i < 4; ++i) g_inazumaBoltAlphas[i] = 0.0f;
}

void OpAnim_Finalize(void)
{
    for (int i = 0; i < 4; ++i)
    {
        if (g_Texture[i]) { g_Texture[i]->Release(); g_Texture[i] = nullptr; }
    }
    if (g_SolidTex) { g_SolidTex->Release(); g_SolidTex = nullptr; }

    // Sprite オブジェクトを解放
    if (g_yakataSprite) { delete g_yakataSprite; g_yakataSprite = nullptr; }
    if (g_ghostSprite) { delete g_ghostSprite; g_ghostSprite = nullptr; }
    if (g_basutaSprite) { delete g_basutaSprite; g_basutaSprite = nullptr; }
    if (g_inazumaSprite) { delete g_inazumaSprite; g_inazumaSprite = nullptr; }

    // 参照をクリア
    g_pDevice = nullptr;
    g_pContext = nullptr;

    SetFPS(60);

}

void OpAnim_Update()
{
    const float screenWidth = (float)Direct3D_GetBackBufferWidth();
    const float screenHeight = (float)Direct3D_GetBackBufferHeight();

    static float timer = 0.0f;
    static bool waitStarted = false;
    static float waitTimer = 0.0f;

    const float delta = 1.0f / 60.0f;
    timer += delta;

    // --- 稲妻ロジック: ランダム間隔で短時間のストライクを作る ---
    // 次の発生までカウントダウン
    g_inazumaNextStrike -= delta;
    if (g_inazumaNextStrike <= 0.0f && !g_inazumaActive)
    {
        // ストライク開始
        g_inazumaActive = true;
        g_inazumaTimer = 0.0f;
        g_inazumaStrikeDuration = 0.06f + Rand01() * 0.18f; // 短いストライク（0.06～0.24s）
        g_inazumaFlash = 0.9f + Rand01() * 0.25f; // 強めの画面フラッシュ
        // 複数ボルトを短時間でちらつかせる
        for (int i = 0; i < 4; ++i) g_inazumaBoltAlphas[i] = 0.5f + Rand01() * 0.6f;
    }

    if (g_inazumaActive)
    {
        g_inazumaTimer += delta;

        // ボルトとフラッシュは開始直後に強く、その後急速に減衰
        float p = g_inazumaTimer / g_inazumaStrikeDuration;
        float fade = 1.0f - EaseOutCubic(p);
        if (fade < 0.0f) fade = 0.0f;

        for (int i = 0; i < 4; ++i) g_inazumaBoltAlphas[i] *= fade;

        // フラッシュは少しだけ短く減らす
        g_inazumaFlash *= fade;

        if (g_inazumaTimer >= g_inazumaStrikeDuration)
        {
            // ストライク終了、次発生をランダム設定
            g_inazumaActive = false;
            g_inazumaTimer = 0.0f;
            g_inazumaNextStrike = 1.5f + Rand01() * 6.0f; // 次は 1.5..7.5 秒
            g_inazumaFlash = 0.0f;
            for (int i = 0; i < 4; ++i) g_inazumaBoltAlphas[i] = 0.0f;
        }
    }

    // タイマーで向きを制御（既存シーケンス）
    if (g_forceFacingByTimer)
    {
        if (timer < 3.0f) g_ghostFacingLeft = false;
        else if (timer < 4.0f) g_ghostFacingLeft = true;
        else g_ghostFacingLeft = false;
    }

    // 右向き（false）になった瞬間に縮小アニメを一度だけ開始（幽霊）
    if (!g_shrinkTriggered && !g_shrinkAppliedOnce &&
        (g_ghostFacingLeft != g_prevGhostFacingLeft) && (g_ghostFacingLeft == false))
    {
        g_shrinkTriggered = true;
        g_shrinkTimer = 0.0f;
    }

    // bikkuri のトリガー：一度だけ（g_bikkuriShownOnce==false 時のみトリガー許可）
    if (!g_bikkuriShownOnce && (g_ghostFacingLeft != g_prevGhostFacingLeft))
    {
        if (!g_bikkuriShown)
        {
            g_bikkuriShown = true;
            g_bikkuriTimer = 0.0f;
            g_bikkuriFlip = g_ghostFacingLeft;
        }
    }

    if (!g_positionsInitialized)
    {
        XMFLOAT2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
        const float leftMargin = 20.0f;
        g_yakataPos.x = (g_imageSize.x / 2.0f) + leftMargin;
        g_yakataPos.y = center.y;
        g_ghostPos.x = g_yakataPos.x + (g_imageSize.x * 0.35f);
        g_ghostPos.y = g_yakataPos.y - 40.0f;
        g_basutaPos.x = SCREEN_WIDTH + (g_imageSize.x / 2.0f) + 100.0f;
        g_basutaPos.y = center.y + 30.0f;
        g_basutaTarget.x = g_yakataPos.x + (g_imageSize.x * 0.25f);
        g_basutaTarget.y = g_yakataPos.y + 20.0f;
        g_positionsInitialized = true; g_basutaMoving = false;
        g_basutaStartFromRight = (g_basutaPos.x > SCREEN_WIDTH);
        g_basutaEnteredScreen = false;
    }

    const float ghostStart = 1.5f;
    const float basutaStart = 0.8f;
    const float fadeDuration = 2.4f;

    if (timer > ghostStart)
    {
        alpha[1] += delta / fadeDuration;
        if (alpha[1] > 1.0f) alpha[1] = 1.0f;
    }

    // basuta 到来前リードで bikkuri を表示（まだ描画確定していなければトリガー）
    if (!g_bikkuriShownOnce && !g_bikkuriShown &&
        timer > (basutaStart - g_bikkuriLeadTime) && timer <= basutaStart)
    {
        g_bikkuriFlip = g_basutaStartFromRight;
        g_bikkuriShown = true; g_bikkuriTimer = 0.0f;
    }

    if (timer > basutaStart)
    {
        alpha[2] += delta / fadeDuration;
        if (alpha[2] > 1.0f) alpha[2] = 1.0f;

        if (!g_basutaMoving && !g_bikkuriShown && !g_bikkuriShownOnce)
        {
            g_bikkuriFlip = (g_basutaTarget.x < g_basutaPos.x);
            g_bikkuriShown = true; g_bikkuriTimer = 0.0f;
        }
        g_basutaMoving = true;
    }

    // basuta 移動処理（簡潔） + 縮小（近づいている風）
    if (g_basutaMoving)
    {
        const float moveSpeed = g_basutaSpeed;
        float dx = g_basutaTarget.x - g_basutaPos.x;
        float dy = g_basutaTarget.y - g_basutaPos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 1.0f)
        {
            float nx = dx / dist, ny = dy / dist;
            float step = moveSpeed * delta;
            if (step >= dist) { g_basutaPos = g_basutaTarget; g_basutaMoving = false; }
            else { g_basutaPos.x += nx * step; g_basutaPos.y += ny * step; }
        }
        else { g_basutaPos = g_basutaTarget; g_basutaMoving = false; }

        // basuta が館へ近づくほど縮小して見えるようにする（線形）
        {
            float dxT = g_basutaTarget.x - g_basutaPos.x;
            float dyT = g_basutaTarget.y - g_basutaPos.y;
            float distToTarget = sqrtf(dxT * dxT + dyT * dyT);
            if (distToTarget < g_basutaShrinkStart)
            {
                float ratio = distToTarget / g_basutaShrinkStart; // 1 -> 0
                g_basutaScale = g_basutaTargetScale + (1.0f - g_basutaTargetScale) * ratio;
            }
            else
            {
                g_basutaScale = 1.0f;
            }
        }

        // 画面入場で幽霊反応（従来ロジック）
        if (!g_basutaEnteredScreen)
        {
            const float enterThresholdRight = SCREEN_WIDTH - (g_imageSize.x * 0.5f);
            const float enterThresholdLeft = (g_imageSize.x * 0.5f);
            if (g_basutaStartFromRight)
            {
                if (g_basutaPos.x <= enterThresholdRight)
                {
                    g_basutaEnteredScreen = true;
                    if (!g_forceFacingByTimer) g_ghostFacingLeft = false;
                    if (g_ghostState == GHOST_IDLE)
                    {
                        g_ghostState = GHOST_ALERT;
                        g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                    }
                }
            }
            else
            {
                if (g_basutaPos.x >= enterThresholdLeft)
                {
                    g_basutaEnteredScreen = true;
                    if (!g_forceFacingByTimer) g_ghostFacingLeft = true;
                    if (g_ghostState == GHOST_IDLE)
                    {
                        g_ghostState = GHOST_ALERT;
                        g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                    }
                }
            }
        }
    }
    else
    {
        // 動いていないときは通常サイズに戻す
        g_basutaScale = 1.0f;
    }

    // bikkuri タイマー（表示中は Update で時間を管理）
    if (g_bikkuriShown)
    {
        g_bikkuriTimer += delta;
        if (g_bikkuriTimer >= g_bikkuriDuration)
        {
            g_bikkuriShown = false;
            g_bikkuriTimer = 0.0f;
            // g_bikkuriShownOnce は描画が確実に行われた後に LogoDraw で true にする
        }
    }

    // 幽霊状態管理（既存ロジックを簡潔に維持）
    {
        const float baseTriggerDist = 320.0f;
        const float triggerDist = baseTriggerDist + (g_basutaSpeed * g_ghostLeadSeconds);
        const float fleeDist = 200.0f;
        float dx = g_basutaPos.x - g_ghostPos.x, dy = g_basutaPos.y - g_ghostPos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (g_ghostState == GHOST_IDLE)
        {
            if (dist <= triggerDist && alpha[2] > 0.3f)
            {
                g_ghostState = GHOST_ALERT;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
                if (!g_basutaEnteredScreen && !g_forceFacingByTimer)
                {
                    g_ghostFacingLeft = (g_basutaPos.x < g_ghostPos.x);
                }
            }
        }
        else if (g_ghostState == GHOST_ALERT)
        {
            if (dist <= fleeDist)
            {
                g_ghostState = GHOST_MOVE_TO_HOUSE;
                float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
                if (!g_forceFacingByTimer) g_ghostFacingLeft = (tx < g_ghostPos.x);
            }
            else
            {
                const float rotSpeed = 3.5f;
                float deltaAngle = AngleDelta(g_ghostTargetAngle, g_ghostAngle);
                float maxStep = rotSpeed * delta;
                if (fabsf(deltaAngle) <= maxStep) g_ghostAngle = g_ghostTargetAngle;
                else g_ghostAngle += (deltaAngle > 0.0f ? 1.0f : -1.0f) * maxStep;
                g_ghostTargetAngle = atan2f(g_basutaPos.y - g_ghostPos.y, g_basutaPos.x - g_ghostPos.x);
            }
        }
        else if (g_ghostState == GHOST_MOVE_TO_HOUSE)
        {
            const float ghostMoveSpeed = 180.0f;
            float tx = g_yakataPos.x - (g_imageSize.x * 0.15f);
            float ty = g_yakataPos.y;
            float dxh = tx - g_ghostPos.x, dyh = ty - g_ghostPos.y;
            float distH = sqrtf(dxh * dxh + dyh * dyh);
            const float startDist = 200.0f;

            if (!g_shrinkAppliedOnce)
            {
                if (distH < startDist) g_ghostScale = 0.3f + (distH / startDist) * 0.7f;
                else g_ghostScale = 1.0f;
            }
            else
            {
                g_ghostScale = g_shrinkTargetScale;
            }

            if (distH > 2.0f)
            {
                float nx = dxh / distH, ny = dyh / distH;
                float step = ghostMoveSpeed * delta;
                if (step >= distH) { g_ghostPos.x = tx; g_ghostPos.y = ty; g_ghostState = GHOST_IDLE; alpha[1] = 0.0f; }
                else
                {
                    g_ghostPos.x += nx * step; g_ghostPos.y += ny * step;
                    float targetMoveAngle = atan2f(ny, nx);
                    float moveDeltaAngle = AngleDelta(targetMoveAngle, g_ghostAngle);
                    float moveRotSpeed = 2.5f;
                    float moveMaxStep = moveRotSpeed * delta;
                    if (fabsf(moveDeltaAngle) <= moveMaxStep) g_ghostAngle = targetMoveAngle;
                    else g_ghostAngle += (moveDeltaAngle > 0.0f ? 1.0f : -1.0f) * moveMaxStep;
                }
            }
            else { g_ghostState = GHOST_IDLE; alpha[1] = 0.0f; }
        }
    }

    // 右向き縮小アニメ更新（移動縮小と排他）
    if (g_shrinkTriggered && !g_shrinkAppliedOnce && g_ghostState != GHOST_MOVE_TO_HOUSE)
    {
        g_shrinkTimer += delta;
        float t = g_shrinkTimer / g_shrinkDuration; if (t > 1.0f) t = 1.0f;
        float e = EaseOutCubic(t);
        g_ghostScale = 1.0f + (g_shrinkTargetScale - 1.0f) * e;
        if (t >= 1.0f) { g_shrinkTriggered = false; g_shrinkAppliedOnce = true; g_ghostScale = g_shrinkTargetScale; }
    }

    if (!fadeStarted && timer > 7.0f && GetFadeState() == FADE_NONE)
    {
        fadeStarted = true;
        // StartFade を使う（プロジェクトの Fade 実装に合わせる）
        StartFade(SCENE_GAME);
    }

    if (fadeStarted && !waitStarted && GetFadeState() == FADE_NONE) { waitStarted = true; waitTimer = 0.0f; }
    if (waitStarted) { waitTimer += delta; if (waitTimer >= 1.5f) SetScene(SCENE_TITLE); }

    // ふわふわ・basuta 揺れ
    if (g_ghostState == GHOST_MOVE_TO_HOUSE)
    {
        g_ghostOffset.y = sinf(timer * 2.0f) * 6.0f;
        g_ghostOffset.x = sinf(timer * 0.7f) * 3.0f;
        g_ghostBobRotation = sinf(timer * 1.2f) * 0.03f;
    }
    else
    {
        g_ghostOffset.y = sinf(timer * 2.5f) * 12.0f;
        g_ghostOffset.x = sinf(timer * 0.9f) * 6.0f;
        g_ghostBobRotation = sinf(timer * 1.2f) * 0.06f;
    }

    if (g_basutaMoving)
    {
        g_basutaOffset.y = sinf(timer * 5.0f) * 8.0f;
        g_basutaOffset.x = sinf(timer * 10.0f) * 3.0f;
    }
    else
    {
        g_basutaOffset.y = sinf(timer * 2.0f) * 4.0f;
        g_basutaOffset.x = sinf(timer * 1.5f) * 2.0f;
    }

    if (Keyboard_IsKeyDownTrigger(KK_E)) SetScene(SCENE_TITLE);

    // フレーム終わりに前フレーム向きを更新（bikkuri トリガーはこの更新より前に行われる）
    g_prevGhostFacingLeft = g_ghostFacingLeft;
}

void OpAnimDraw(void)
{
    const float screenWidth = (float)Direct3D_GetBackBufferWidth();
    const float screenHeight = (float)Direct3D_GetBackBufferHeight();

    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, screenWidth, screenHeight, 0.0f, 0.0f, 1.0f));
    SetBlendState(BLENDSTATE_ALFA);

    XMFLOAT2 center = { screenWidth / 2.0f, screenHeight / 2.0f };

    // 仮想解像度（ここを 1280x720 に固定）
    const float virtualW = 1280.0f;
    const float virtualH = 720.0f;

    // 実スクリーンに合わせた等倍スケール（アスペクト比を保つ）
    float scale = std::min(screenWidth / virtualW, screenHeight / virtualH);
    XMFLOAT2 virtualSize = { virtualW * scale, virtualH * scale };
    XMFLOAT2 virtualCenter = { screenWidth * 0.5f, screenHeight * 0.5f };

    if (g_SolidTex)
    {
        g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);

        // 中央の "仮想キャンバス" を塗る（紫）
        XMFLOAT4 purple = { 0.45f, 0.10f, 0.45f, 1.0f };
        Sprite_Single_Draw(virtualCenter, virtualSize, 0.0f, purple, BLENDSTATE_ALFA, g_SolidTex);

        // 仮想キャンバス外側を黒で塗りつぶしてレターボックス／ピラーボックスを実現
        XMFLOAT4 black = { 0.0f, 0.0f, 0.0f, 1.0f };

        float verticalBorder = (screenHeight - virtualSize.y) * 0.5f;
        float horizontalBorder = (screenWidth - virtualSize.x) * 0.5f;

        if (verticalBorder > 0.0f)
        {
            // 上
            XMFLOAT2 topSize = { screenWidth, verticalBorder };
            XMFLOAT2 topPos = { screenWidth * 0.5f, verticalBorder * 0.5f };
            Sprite_Single_Draw(topPos, topSize, 0.0f, black, BLENDSTATE_ALFA, g_SolidTex);

            // 下
            XMFLOAT2 bottomPos = { screenWidth * 0.5f, screenHeight - verticalBorder * 0.5f };
            Sprite_Single_Draw(bottomPos, topSize, 0.0f, black, BLENDSTATE_ALFA, g_SolidTex);
        }

        if (horizontalBorder > 0.0f)
        {
            // 左
            XMFLOAT2 leftSize = { horizontalBorder, virtualSize.y };
            XMFLOAT2 leftPos = { horizontalBorder * 0.5f, virtualCenter.y };
            Sprite_Single_Draw(leftPos, leftSize, 0.0f, black, BLENDSTATE_ALFA, g_SolidTex);

            // 右
            XMFLOAT2 rightPos = { screenWidth - horizontalBorder * 0.5f, virtualCenter.y };
            Sprite_Single_Draw(rightPos, leftSize, 0.0f, black, BLENDSTATE_ALFA, g_SolidTex);
        }
    }

    // 以下は既存の描画処理をそのまま維持（屋敷・幽霊・basuta・bikkuri）
    // 屋敷
    if (g_Texture[0])
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[0]);
        Sprite_Single_Draw(g_yakataPos, g_imageSize, 0.0f, XMFLOAT4{ 1,1,1,1 }, BLENDSTATE_ALFA, g_Texture[0]);
    }

    // 幽霊
    if (alpha[1] > 0.0f && g_Texture[1])
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[1]);
        XMFLOAT4 ghostCol = { 1.0f,1.0f,1.0f,alpha[1] };
        XMFLOAT2 drawPos = { g_ghostPos.x + g_ghostOffset.x, g_ghostPos.y + g_ghostOffset.y };
        XMFLOAT2 scaledSize = { g_imageSize.x * g_ghostScale, g_imageSize.y * g_ghostScale };
        FLIPTYPE2D flip = g_ghostFacingLeft ? FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL : FLIPTYPE2D::FLIPTYPE2D_NONE;
        Sprite_Single_Draw(drawPos, scaledSize, 0.0f, ghostCol, BLENDSTATE_ALFA, g_Texture[1], flip);
    }

    // basuta（スケール適用）
    if (alpha[2] > 0.0f && g_Texture[2])
    {
        g_pContext->PSSetShaderResources(0, 1, &g_Texture[2]);
        XMFLOAT4 col2 = { 1,1,1,alpha[2] };
        XMFLOAT2 drawPos = { g_basutaPos.x + g_basutaOffset.x, g_basutaPos.y + g_basutaOffset.y };
        XMFLOAT2 basutaSize = { g_imageSize.x * g_basutaScale, g_imageSize.y * g_basutaScale };
        Sprite_Single_Draw(drawPos, basutaSize, 0.0f, col2, BLENDSTATE_ALFA, g_Texture[2]);
    }

    // bikkuri
    if (g_bikkuriShown && g_Texture[3])
    {
        XMFLOAT2 bsize = { 180.0f, 180.0f };
        XMFLOAT2 bpos;
        bpos.x = g_ghostPos.x + g_ghostOffset.x;
        bpos.y = g_ghostPos.y + g_ghostOffset.y + (g_imageSize.y * 0.45f);

        float halfH = bsize.y * 0.5f;
        const float margin = 8.0f;
        if (bpos.y + halfH + margin > screenHeight) bpos.y = screenHeight - halfH - margin;
        if (bpos.y - halfH - margin < 0.0f) bpos.y = halfH + margin;

        if (g_SolidTex)
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
            XMFLOAT4 dbgBg = { 0.0f,0.0f,0.0f,0.45f };
            XMFLOAT2 bgSize = { bsize.x + 20.0f, bsize.y + 20.0f };
            Sprite_Single_Draw({ bpos.x, bpos.y }, bgSize, 0.0f, dbgBg, BLENDSTATE_ALFA, g_SolidTex);
        }

        g_pContext->PSSetShaderResources(0, 1, &g_Texture[3]);
        FLIPTYPE2D bflip = g_bikkuriFlip ? FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL : FLIPTYPE2D::FLIPTYPE2D_NONE;
        Sprite_Single_Draw(bpos, bsize, 0.0f, XMFLOAT4{ 1,1,1,1 }, BLENDSTATE_ALFA, g_Texture[3], bflip);

        if (!g_bikkuriShownOnce) g_bikkuriShownOnce = true;
    }

    // --- 稲妻描画: 画面右端に複数ボルト＋画面フラッシュ ---
    {
        // 右端位置（仮想キャンバスの右端に寄せる／調整可）
        const float marginFromEdge = 60.0f;
        XMFLOAT2 inazumaPos = { screenWidth - marginFromEdge, virtualCenter.y - 80.0f }; // 少し上寄せ
        // テクスチャは縦長想定、スケールで大きさ調整
        XMFLOAT2 inazumaSize = { 220.0f, 720.0f };

        // 画面全体の強いフラッシュ（白）を重ねる
        if (g_inazumaFlash > 0.001f && g_SolidTex)
        {
            float flashAlpha = std::min(g_inazumaFlash, 1.0f);
            g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
            XMFLOAT4 flashCol = { 1.0f, 0.98f, 0.9f, flashAlpha * 0.7f }; // 少し暖色寄せ
            XMFLOAT2 full = { screenWidth, screenHeight };
            XMFLOAT2 fullCenter = { screenWidth * 0.5f, screenHeight * 0.5f };
            Sprite_Single_Draw(fullCenter, full, 0.0f, flashCol, BLENDSTATE_ALFA, g_SolidTex);
        }

        // 稲妻本体（テクスチャ）を複数回、微妙にオフセットして描画 -> 煌めき感
        if (g_Texture[3])
        {
            g_pContext->PSSetShaderResources(0, 1, &g_Texture[3]);
            // ボルトごとに異なるアルファとオフセット
            for (int i = 0; i < 4; ++i)
            {
                float a = g_inazumaBoltAlphas[i];
                if (a <= 0.003f) continue;
                // オフセット幅を決める（稲妻の「ジグザグ」を表現）
                float xo = (i - 1.5f) * 12.0f + (Rand01() - 0.5f) * 8.0f;
                float yo = (Rand01() - 0.5f) * 20.0f;
                XMFLOAT2 pos = { inazumaPos.x + xo, inazumaPos.y + yo };
                XMFLOAT2 size = inazumaSize;
                // 稲妻が強く見えるように少し明るめの色を乗算的に表現（アルファで調整）
                XMFLOAT4 col = { 1.0f, 1.0f, 0.92f, a };
                Sprite_Single_Draw(pos, size, 0.0f, col, BLENDSTATE_ALFA, g_Texture[3]);
            }
        }

        // 微小な白いラインを数本描く（単色テクスチャを使用）
        if (g_SolidTex)
        {
            g_pContext->PSSetShaderResources(0, 1, &g_SolidTex);
            // 数本のラインをランダムに描画して「鋭さ」を追加（短時間で alpha を下げる）
            for (int i = 0; i < 3; ++i)
            {
                float a = (g_inazumaBoltAlphas[i] > 0.0f) ? (g_inazumaBoltAlphas[i] * 0.6f) : 0.0f;
                if (a <= 0.003f) continue;
                float xo = -6.0f + i * 10.0f + (Rand01() - 0.5f) * 6.0f;
                float length = 320.0f + Rand01() * 220.0f;
                XMFLOAT2 lineSize = { 6.0f, length };
                XMFLOAT2 linePos = { inazumaPos.x + xo, inazumaPos.y + (length * 0.5f) - 60.0f };
                XMFLOAT4 col = { 1.0f, 1.0f, 0.95f, a };
                Sprite_Single_Draw(linePos, lineSize, 0.0f, col, BLENDSTATE_ALFA, g_SolidTex);
            }
        }
    }
}