struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float3 fragPos  : POSITION;
    float3 texcoord : TEXCOORD;
};

TextureCube environmentMap;
SamplerState g_sampler;

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return float4(environmentMap.Sample(g_sampler, input.fragPos).rgb, 1);
}
