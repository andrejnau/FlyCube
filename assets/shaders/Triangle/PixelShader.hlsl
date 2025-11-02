struct VsOutput {
    float4 pos : SV_POSITION;
};

struct ConstantLayout {
    float4 color;
};

ConstantBuffer<ConstantLayout> constant_buffer : register(b0, space0);

float4 main(VsOutput input) : SV_TARGET
{
   return constant_buffer.color;
}
