cbuffer VSParams
{
    float4x4 World;
};

struct VertexInput
{
    float3 Position   : SV_POSITION;
    float2 texCoord   : TEXCOORD;
    uint bones_offset : BONES_OFFSET;
    uint bones_count  : BONES_COUNT;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord  : TEXCOORD;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(float4(input.Position, 1.0), World);
    output.pos = worldPosition;
    output.texCoord = input.texCoord;
    return output;
}
