struct VS_INPUT
{
    float3 pos       : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
};

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

    float4 worldPos = mul(float4(vs_in.pos, 1.0), model);
    vs_out.fragPos = worldPos.xyz;
    vs_out.pos = mul(worldPos, mul(view, projection));;
    vs_out.texCoord = vs_in.texCoord;

    vs_out.normal = mul(vs_in.normal, (float3x3)normalMatrix);
    vs_out.tangent = mul(vs_in.tangent, (float3x3)normalMatrix);

    return vs_out;
}
