struct VS_INPUT
{
    float3 pos       : POSITION;
    float3 normal    : NORMAL;
    float2 texCoord  : TEXCOORD;
    float3 tangent   : TANGENT;
    uint bones_offset : TEXCOORD1;
    uint bones_count  : TEXCOORD2;
};

struct BoneInfo
{
    uint bone_id;
    float weight;
};

StructuredBuffer<BoneInfo> bone_info;
StructuredBuffer<float4x4> gBones;

cbuffer ConstantBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 normalMatrix;
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

    float4 pos = float4(0, 0, 0, 0);
    for (uint i = 0; i < vs_in.bones_count; ++i)
    {
        pos += bone_info[i + vs_in.bones_offset].weight * mul(float4(vs_in.pos, 1.0), gBones[bone_info[i + vs_in.bones_offset].bone_id]);
    }
    if (vs_in.bones_count == 0)
    {
        pos = float4(vs_in.pos, 1.0);
    }
    float4 worldPos = mul(pos, model);
    vs_out.fragPos = worldPos.xyz;
    vs_out.pos = mul(worldPos, mul(view, projection));
    vs_out.texCoord = vs_in.texCoord;

    vs_out.normal = mul(vs_in.normal, (float3x3)normalMatrix);
    vs_out.tangent = mul(vs_in.tangent, (float3x3)normalMatrix);

    return vs_out;
}
