/*==============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/
Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

struct LIGHT
{
    bool enable;
    bool3 dummy;
    float4 Direction;
    float4 Diffuse;
    float4 Ambient;
};

cbuffer Buffer2 : register(b2)
{
    LIGHT Light;
};

cbuffer Buffer3 : register(b3)
{
    float4 MaterialColor;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float4 normal : NORMAL0;        // 法線（ワールド座標系）
    float4 worldPos : TEXCOORD1;    // ワールド座標
};

float4 main(PS_INPUT ps_in) : SV_TARGET
{
    // マテリアル色とテクスチャを組み合わせる
    // テクスチャサンプル
    float4 texColor = g_Texture.Sample(g_SamplerState, ps_in.texcoord);
    
    // ベースカラー決定：テクスチャがない場合はマテリアル色を使用
    float4 baseColor = texColor * MaterialColor * ps_in.color;
    
    // ライティング計算（Lambert）
    if (Light.enable)
    {
        // 法線を正規化
        float3 normal = normalize(ps_in.normal.xyz);
        
        // ライト方向を正規化
        float3 lightDir = normalize(-Light.Direction.xyz);
        
        // Lambert項の計算
        float lambert = max(dot(normal, lightDir), 0.0f);
        
        // ディフューズライティング
        float3 diffuse = lambert * Light.Diffuse.rgb;
        
        // アンビエント（環境光）の追加
        float3 ambient = Light.Ambient.rgb;
        
        // 最終的なライティングカラーの合成
        baseColor.rgb *= (diffuse + ambient);
    }
    
    return baseColor;
}

//float4 main() : SV_TARGET
//{
//    return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}
