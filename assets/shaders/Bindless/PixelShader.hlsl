struct Settings
{
    float4 color;
};

ConstantBuffer<Settings> constant_buffer: register(b1, space0);

float4 main(float4 pos : SV_POSITION) : SV_TARGET
{
   return constant_buffer.color;
}
