cbuffer Params
{
    float4x4 World;
};

struct BoneInfo
{
    uint bone_id;
    float weight;
};

StructuredBuffer<BoneInfo> bone_info;
StructuredBuffer<float4x4> gBones;

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
    float4x4 transform = {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 }
    };
    for (uint i = 0; i < input.bones_count; ++i)
    {
        transform += bone_info[i + input.bones_offset].weight * gBones[bone_info[i + input.bones_offset].bone_id];
    }
    if (input.bones_count == 0)
    {
        float4x4 transform_identity = {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 }
        };
        transform = transform_identity;
    }

    VertexOutput output = (VertexOutput) 0;
    float4 worldPosition = mul(mul(float4(input.Position, 1.0), transform), World);
    output.pos = worldPosition;
    output.texCoord = input.texCoord;
    return output;
}
