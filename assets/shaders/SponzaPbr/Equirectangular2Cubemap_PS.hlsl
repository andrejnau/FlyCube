struct VS_OUTPUT
{
    float4 pos     : SV_POSITION;
    float3 fragPos : POSITION;
    uint RTIndex   : SV_RenderTargetArrayIndex;
};

Texture2D equirectangularMap;
SamplerState g_sampler;

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;

float2 SampleSphericalMap(float3 v)
{
    float phi   = atan2(v.z, v.x);
    float theta = acos(v.y);
    return float2(phi / TwoPI, theta / PI);
}

float4 main(VS_OUTPUT input) : SV_TARGET
{		
    float2 uv = SampleSphericalMap(normalize(input.fragPos));
    float3 color = equirectangularMap.Sample(g_sampler, uv).rgb;
    return float4(color, 1.0);
}
