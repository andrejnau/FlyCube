struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 texCoord: TEXCOORD;
};

cbuffer ConstantBuffer : register(b1)
{
    float4 colorMultiplier;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.texCoord);
}