struct VS_INPUT
{
    float4 pos : POSITION;
    float4 color: COLOR;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float4 color: COLOR;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 wvpMat;
};

cbuffer ConstantBuffer : register(b1)
{
    float4 colorMultiplier;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(input.pos, wvpMat);
    output.color = input.color * colorMultiplier;
    return output;
}