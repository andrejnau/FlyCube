struct VS_INPUT
{
    float3 pos : POSITION;
    float3 texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.texcoord = input.texcoord;
    output.pos = float4(input.pos, 1.0);
    return output;
}
