struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

#ifndef SAMPLE_COUNT
#define SAMPLE_COUNT 1
#endif

#if SAMPLE_COUNT > 1
#define TEXTURE_TYPE Texture2DMS<float4>
#else
#define TEXTURE_TYPE Texture2D
#endif

TEXTURE_TYPE gPosition;
TEXTURE_TYPE gNormal;
TEXTURE_TYPE gAlbedo;
TEXTURE_TYPE gMaterial;
Texture2D gSSAO;
TextureCube irradianceMap;
TextureCube prefilterMap;
Texture2D brdfLUT;

SamplerState g_sampler : register(s0);
SamplerState brdf_sampler : register(s1);

static const float PI = acos(-1.0);

#define LIGHT_COUNT 90

cbuffer Light
{
    float4 light_color[LIGHT_COUNT];
    float4 light_pos[LIGHT_COUNT];
    float4 viewPos;
};

cbuffer Settings
{
    bool use_ao;
    bool use_ssao;
    bool use_IBL_diffuse;
    bool use_IBL_specular;
    bool only_ambient;
    bool use_spec_ao_by_ndotv_roughness;
    bool show_only_normal;
    float ambient_power;
};

float4 getTexture(TEXTURE_TYPE _texture, float2 _tex_coord, int ss_index, bool _need_gamma = false)
{
#if SAMPLE_COUNT > 1
    float3 gbufferDim;
    _texture.GetDimensions(gbufferDim.x, gbufferDim.y, gbufferDim.z);
    float2 texcoord = _tex_coord * float2(gbufferDim.xy);
    float4 _color = _texture.Load(texcoord, ss_index);
#else
    float4 _color = _texture.Sample(g_sampler, _tex_coord);
#endif
    if (_need_gamma)
        _color = float4(pow(abs(_color.rgb), 2.2), _color.a);
    return _color;
}

struct Material
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 f0;
};

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max((float3)(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 CookTorrance_GGX(float3 fragPos, float3 n, float3 v, Material m, int i)
{
    n = normalize(n);
    v = normalize(v);
    float3 l = normalize(light_pos[i] - fragPos);
    float3 h = normalize(v + l);

    float distance = length(light_pos[i] - fragPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = light_color[i] * attenuation;

    float NDF = DistributionGGX(n, h, m.roughness);
    float G = GeometrySmith(n, v, l, m.roughness);
    float3 F = FresnelSchlick(max(dot(h, v), 0.0), m.f0);

    float3 nominator = NDF * G * F;
    float denominator = 4 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.001;
    float3 specular = nominator / denominator;

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - m.metallic;

    float NdotL = max(dot(n, l), 0.0);

    return (kD * m.albedo / PI + specular) * radiance * NdotL;
}

float computeSpecOcclusion(float NdotV, float AO, float roughness)
{
    if (use_spec_ao_by_ndotv_roughness)
        return saturate(pow(NdotV + AO, exp2(-16.0f * roughness - 1.0f)) - 1.0f + AO);
    return AO;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 lighting = 0;
    [unroll]
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float3 fragPos = getTexture(gPosition, input.texcoord, i).rgb;
        float3 normal = normalize(getTexture(gNormal, input.texcoord, i).rgb);
        float3 albedo = getTexture(gAlbedo, input.texcoord, i).rgb;
        float roughness = getTexture(gMaterial, input.texcoord, i).r;
        float metallic = getTexture(gMaterial, input.texcoord, i).g;

        if (show_only_normal)
        {
            lighting += normal;
            break;
        }

        float ao = 1;
        if (use_ao)
            ao = getTexture(gMaterial, input.texcoord, i).b;

        float ssao = 1;
        if (use_ssao)
            ssao = gSSAO.Sample(g_sampler, input.texcoord).r;

        ao = min(ao, ssao);

        Material m;
        m.albedo = albedo;
        m.roughness = roughness;
        m.metallic = metallic;
        m.f0 = lerp(0.04, albedo, metallic);

        float3 V = normalize(viewPos - fragPos);
        float3 R = reflect(-V, normal);

        [unroll]
        for (int i = 0; i < LIGHT_COUNT; ++i)
        {
            lighting += CookTorrance_GGX(fragPos, normal, V, m, i);
        }

        float3 F = FresnelSchlickRoughness(max(dot(normal, V), 0.0), m.f0, roughness);

        float3 ambient = 0;      
        if (use_IBL_diffuse)
        {
            // ambient lighting (we now use IBL as the ambient term)
            float3 kS = F;
            float3 kD = 1.0 - kS;
            kD *= 1.0 - metallic;
            float3 irradiance = irradianceMap.Sample(g_sampler, normal).rgb;
            float3 diffuse = irradiance * albedo;
            ambient = (kD * diffuse) * ao;
        }

        if (use_IBL_specular)
        {
            // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
            uint width, height, levels;
            prefilterMap.GetDimensions(0, width, height, levels);
            float3 prefilteredColor = prefilterMap.SampleLevel(g_sampler, R, roughness * levels).rgb;
            float2 brdf = brdfLUT.Sample(brdf_sampler, float2(max(dot(normal, V), 0.0), roughness)).rg;
            float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
            ambient += specular * computeSpecOcclusion(max(dot(normal, V), 0.0), ao, roughness);
        }

        if (!use_IBL_diffuse && !use_IBL_specular)
            ambient = 0.03 * albedo * ao;

        if (only_ambient)
            lighting = 0;

        lighting += ambient_power * ambient;
    }
    lighting /= SAMPLE_COUNT;

    return float4(lighting, 1.0);
}
