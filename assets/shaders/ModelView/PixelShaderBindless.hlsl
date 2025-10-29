struct VsOutput
{
    float4 pos      : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

struct ConstantLayout
{
    uint32_t base_color;
};

Texture2D bindless_textures[] : register(t0, space1);
SamplerState anisotropic_sampler : register(s1, space0);
ConstantBuffer<ConstantLayout> cbv : register(b2, space0);

float4 main(VsOutput input) : SV_TARGET
{
    float4 base_color = bindless_textures[cbv.base_color].Sample(anisotropic_sampler, input.texcoord);
    return float4(base_color.rgb, 1.0);
}
