struct VsOutput {
    float4 pos      : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

struct ConstantLayout {
    uint32_t base_color_texture;
    uint32_t min_mag_linear_mip_nearest_sampler;
};

Texture2D bindless_textures[] : register(t0, space1);
SamplerState bindless_samplers[] : register(s0, space2);
ConstantBuffer<ConstantLayout> constant_buffer : register(b1, space0);

float4 main(VsOutput input) : SV_TARGET
{
    float4 base_color = bindless_textures[constant_buffer.base_color_texture].Sample(bindless_samplers[constant_buffer.min_mag_linear_mip_nearest_sampler], input.texcoord);
    return float4(base_color.rgb, 1.0);
}
