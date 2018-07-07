struct VS_INPUT
{
    float3 pos        : POSITION;
    float3 normal     : NORMAL;
    float2 texCoord   : TEXCOORD;
    float3 tangent    : TANGENT;
    uint bones_offset : BONES_OFFSET;
    uint bones_count  : BONES_COUNT;
};

struct BoneInfo
{
    uint bone_id;
    float weight;
};

StructuredBuffer<BoneInfo> bone_info;
StructuredBuffer<float4x4> gBones;

cbuffer ConstantBuf
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 normalMatrix;
    float4x4 normalMatrixView;
    float4x4 clip_matrix;
};

struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;
    float4x4 transform = {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 }
    };
    for (uint i = 0; i < vs_in.bones_count; ++i)
    {
        transform += bone_info[i + vs_in.bones_offset].weight * gBones[bone_info[i + vs_in.bones_offset].bone_id];
    }
    if (vs_in.bones_count == 0)
    {
        float4x4 transform_identity = {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 }
        };
        transform = transform_identity;
    }
    float4 pos = mul(float4(vs_in.pos, 1.0), transform);
    float4 worldPos = mul(pos, model);
    vs_out.fragPos = worldPos.xyz;
    vs_out.pos = mul(worldPos, mul(view, mul(projection, clip_matrix)));
    vs_out.texCoord = vs_in.texCoord;
    vs_out.normal = mul(mul(vs_in.normal, transform), (float3x3)normalMatrix);
    vs_out.tangent = mul(mul(vs_in.tangent, transform), (float3x3)normalMatrix);
    return vs_out;
}
