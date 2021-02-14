struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
};

Texture2DMS<uint2> stencilBuffer;

float4 main(VS_OUTPUT input) : SV_TARGET
{
    uint width, height, ms;
    stencilBuffer.GetDimensions(width, height, ms);
    uint2 stenctil = stencilBuffer.Load(uint3(input.texCoord * uint2(width, height), 0), 0);
    return float4(stenctil.g, 1, 0, 1);
}
