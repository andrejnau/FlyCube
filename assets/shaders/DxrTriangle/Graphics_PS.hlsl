struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D tex;

float4 main(VS_OUTPUT input) : SV_TARGET
{
    uint width, height;
    tex.GetDimensions(width, height);
    float3 color = tex.Load(uint3(input.texCoord * uint2(width, height), 0)).rgb;
    return float4(color, 1);
}
