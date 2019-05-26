struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
};

cbuffer Settings
{
    float4 color;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
   return color;
}
