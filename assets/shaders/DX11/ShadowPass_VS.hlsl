cbuffer Params
{
    float4x4 World;
};

struct VertexInput
{
    float3 Position : SV_POSITION;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float3 light_pos : LIGHTPOS;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output = (VertexOutput) 0;
    float4 worldPosition = mul(float4(input.Position, 1.0), World);
    output.Position = worldPosition;
    return output;
}
