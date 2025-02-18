struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
};

cbuffer Settings : register(b0, space0)
{
    float4 color;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
   return color;
}
