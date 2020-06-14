struct VS_OUTPUT
{
    float4 pos     : SV_POSITION;
    float3 fragPos : POSITION;
    uint RTIndex   : SV_RenderTargetArrayIndex;
};

TextureCube environmentMap;
SamplerState g_sampler;

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return float4(environmentMap.Sample(g_sampler, float3(input.fragPos.x, input.fragPos.y, -input.fragPos.z)).rgb, 0);
}
