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
    // テクスチャをサンプル
    float4 texColor = g_Texture.Sample(g_SamplerState, ps_in.texcoord);
    
    // マテリアルカラーが無効な場合（全て0の場合）は白にリセット
    float4 materialColorSafe = MaterialColor;
    if (MaterialColor.r == 0.0f && MaterialColor.g == 0.0f && MaterialColor.b == 0.0f)
    {
        materialColorSafe = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // ベースカラー = テクスチャカラー × マテリアルカラー × 頂点カラー
    float4 baseColor = texColor * materialColorSafe * ps_in.color;
    
    // ライティング計算（Lambert）
    if (Light.enable)
    {
        // 法線を正規化
        float3 normal = normalize(ps_in.normal.xyz);
        
        // ライト方向を正規化
        float3 lightDir = normalize(-Light.Direction.xyz);
        
        // Lambert値の計算
        float lambert = max(dot(normal, lightDir), 0.0f);
        
        // ディフューズライティング
        float3 diffuse = lambert * Light.Diffuse.rgb;
        
        // アンビエント（環境光）の追加
        float3 ambient = Light.Ambient.rgb;
        
        // 最終的なライティングカラーの算出
        baseColor.rgb *= (diffuse + ambient);
    }
    
    return baseColor;
}

//float4 main() : SV_TARGET
//{
//    return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}
