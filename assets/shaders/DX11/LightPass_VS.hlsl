struct VS_INPUT
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent  : TANGENT;
};

struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}
