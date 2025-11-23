/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer Buffer0 : register(b0)
{
    float4x4 mtx;
};

cbuffer Buffer1 : register(b1)
{
    float4x4 worldMtx;
};

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

//入力用頂点構造体
struct VS_INPUT
{
    float4 posL : POSITION0;
    float4 normal : NORMAL0;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
	
};

//出力用頂点構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;
    
    //頂点を行列で変換
    vs_out.posH = mul(vs_in.posL, mtx);
    //t頂点Colorはそのまま出力
    vs_out.color = vs_in.color;
    
    vs_out.texcoord = vs_in.texcoord;
   
    if (Light.enable == true)
    {
        //法線をワールド変換
        float4 normal = float4(vs_in.normal.xyz, 0.0f);//法線をコピー
        normal = mul(normal, worldMtx); //ワールド変換
        normal = normalize(normal); //正規化
        
        //ライティング
        float light = -dot(normal.xyz, -Light.Direction.xyz);
        light = saturate(light); //0～1にクランプ 飽和演算
        vs_out.color.rgb *= light;
        vs_out.color.rgb += Light.Ambient.rgb; //環境光を加算
    }
    
    
    return vs_out;
}

//=============================================================================
// 頂点シェーダ
//=============================================================================







//float4 main(in float4 posL : POSITION0 ) : SV_POSITION
//{
//	return mul(posL, mtx);
//}

