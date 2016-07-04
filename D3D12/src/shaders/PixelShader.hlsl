struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
    float3 bitangent: BITANGENT;
};

cbuffer ConstantBuffer : register(b1)
{
    float4 colorMultiplier;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
   //return g_texture.Sample(g_sampler, input.texCoord);
   return float4(input.normal, 1.0);
}