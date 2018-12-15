struct VS_INPUT
{
    float3 pos : POSITION;
};

struct VS_OUTPUT
{
    float4 pos     : SV_POSITION;
    float3 fragPos : POSITION;
    uint RTIndex   : SV_RenderTargetArrayIndex;
};

cbuffer ConstantBuf
{
    float4x4 view;
    float4x4 projection;
    uint face;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT vs_out;
    vs_out.fragPos = input.pos;
    vs_out.pos = mul(float4(input.pos, 1.0), mul(view, projection));
    vs_out.RTIndex = face;
    return vs_out;
}
