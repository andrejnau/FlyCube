cbuffer Params
{
    float4x4 World;
};

struct VertexInput
{
    float3 Position : SV_POSITION;
    float2 texCoord  : TEXCOORD;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord  : TEXCOORD;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output = (VertexOutput) 0;
    float4 worldPosition = mul(float4(input.Position, 1.0), World);
    output.pos = worldPosition;
    output.texCoord = input.texCoord;
    return output;
}
