struct VsOutput
{
    float4 pos      : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D base_color_texture : register(t0, space1);
SamplerState anisotropic_sampler : register(s1, space0);

float4 main(VsOutput input) : SV_TARGET
{
    float4 base_color = base_color_texture.Sample(anisotropic_sampler, input.texcoord);
    return float4(base_color.rgb, 1.0);
}
