struct VsOutput {
    float4 pos      : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

struct ConstantLayout {
    float4x4 mvp;
};

ConstantBuffer<ConstantLayout> constant_buffer : register(b0, space0);

VsOutput main(float3 pos : POSITION, float2 texcoord : TEXCOORD)
{
    VsOutput output;
    output.pos = mul(float4(pos, 1.0), constant_buffer.mvp);
    output.texcoord = texcoord;
    return output;
}
