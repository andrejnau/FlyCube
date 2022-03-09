struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
   return float4(1, 0, 0, 1);
}
