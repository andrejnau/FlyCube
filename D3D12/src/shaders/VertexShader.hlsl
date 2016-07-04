struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
    float3 bitangent: BITANGENT;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
    float3 bitangent: BITANGENT;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 mvp;
    float3 lightPos;
    float3 viewPos;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(float4(input.pos, 1.0), mvp);
    output.texCoord = input.texCoord;

    float3x3 normalMatrix = (float3x3)transpose(model);
    float3 N = normalize(mul(input.normal, normalMatrix));

    output.normal = N;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;
    return output;
}