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
TextureCube<float> LightCubeShadowMap;

SamplerState g_sampler : register(s0);
SamplerState brdf_sampler : register(s1);
SamplerComparisonState LightCubeShadowComparsionSampler : register(s2);

static const float PI = acos(-1.0);

#define LIGHT_COUNT 90

cbuffer Light
{
    float4 light_color[LIGHT_COUNT];
    float4 light_pos[LIGHT_COUNT];
    float3 viewPos;
};

cbuffer ShadowParams
{
    float s_near;
    float s_far;
    float s_size;
    bool use_shadow;
    float3 shadow_light_pos;
};

cbuffer Settings
{
    bool use_ao;
    bool use_ssao;
    bool use_IBL_diffuse;
    bool use_IBL_specular;
    bool only_ambient;
    bool use_spec_ao_by_ndotv_roughness;
    float ambient_power;
    float light_power;
    bool show_only_albedo;
    bool show_only_normal;
    bool show_only_roughness;
    bool show_only_metalness;
    bool show_only_ao;
    bool use_f0_with_roughness;
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

float _vectorToDepth(float3 vec, float n, float f)
{
    float3 AbsVec = abs(vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    float NormZComp = (f + n) / (f - n) - (2 * f * n) / (f - n) / LocalZcomp;
    return NormZComp;
}

float _sampleCubeShadowHPCF(float3 L, float3 vL)
{
    float sD = _vectorToDepth(vL, s_near, s_far);
    return LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, float3(L.xy, -L.z), sD).r;
}

float _sampleCubeShadowPCFSwizzle3x3(float3 L, float3 vL)
{
    float sD = _vectorToDepth(vL, s_near, s_far);

    float3 forward = float3(L.xy, -L.z);
    float3 right = float3(forward.z, -forward.x, forward.y);
    right -= forward * dot(right, forward);
    right = normalize(right);
    float3 up = cross(right, forward);

    float tapoffset = (1.0f / s_size);

    right *= tapoffset;
    up *= tapoffset;

    float3 v0;
    v0.x = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward - right - up, sD).r;
    v0.y = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward - up, sD).r;
    v0.z = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward + right - up, sD).r;

    float3 v1;
    v1.x = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward - right, sD).r;
    v1.y = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward, sD).r;
    v1.z = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward + right, sD).r;

    float3 v2;
    v2.x = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward - right + up, sD).r;
    v2.y = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward + up, sD).r;
    v2.z = LightCubeShadowMap.SampleCmpLevelZero(LightCubeShadowComparsionSampler, forward + right + up, sD).r;


    return dot(v0 + v1 + v2, .1111111f);
}

// UE4: https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/ShadowProjectionCommon.usf
static const float2 DiscSamples5[] =
{
    // 5 random points in disc with radius 2.500000
    float2(0.000000, 2.500000),
    float2(2.377641, 0.772542),
    float2(1.469463, -2.022543),
    float2(-1.469463, -2.022542),
    float2(-2.377641, 0.772543),
};

float _sampleCubeShadowPCFDisc5(float3 L, float3 vL)
{
    float3 SideVector = normalize(cross(L, float3(0, 0, 1)));
    float3 UpVector = cross(SideVector, L);

    SideVector *= 1.0 / s_size;
    UpVector *= 1.0 / s_size;

    float sD = _vectorToDepth(vL, s_near, s_far);

    float3 nlV = float3(L.xy, -L.z);

    float totalShadow = 0;

    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        float3 SamplePos = nlV + SideVector * DiscSamples5[i].x + UpVector * DiscSamples5[i].y;
        totalShadow += LightCubeShadowMap.SampleCmpLevelZero(
            LightCubeShadowComparsionSampler,
            SamplePos,
            sD);
    }
    totalShadow /= 5;

    return totalShadow;
}

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

float3 CookTorrance_GGX(float3 fragPos, float3 n, float3 v, Material m, float3 light_pos, float3 light_color, bool is_sun = false)
{
    n = normalize(n);
    v = normalize(v);
    float3 l = normalize(light_pos - fragPos);
    float3 h = normalize(v + l);

    float distance = length(light_pos - fragPos);
    float attenuation = 1.0 / (distance * distance);
    if (is_sun)
        attenuation = 1.0;
    float3 radiance = light_power * light_color * attenuation;

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
        return saturate(pow(abs(NdotV + AO), exp2(-16.0f * roughness - 1.0f)) - 1.0f + AO);
    return AO;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 lighting = 0;
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float3 fragPos = getTexture(gPosition, input.texcoord, i).rgb;
        float3 normal = normalize(getTexture(gNormal, input.texcoord, i).rgb);
        float3 albedo = getTexture(gAlbedo, input.texcoord, i).rgb;
        float roughness = getTexture(gMaterial, input.texcoord, i).r;
        float metallic = getTexture(gMaterial, input.texcoord, i).g;

        float ao = 1;
        if (use_ao)
            ao = getTexture(gMaterial, input.texcoord, i).b;

        float ssao = 1;
        if (use_ssao)
            ssao = gSSAO.Sample(g_sampler, input.texcoord).r;

        ao = pow(max(0, min(ao, ssao)), 2.2);

        if (show_only_albedo)
        {
            lighting += albedo;
            continue;
        }
        else if (show_only_normal)
        {
            lighting += normal;
            continue;
        }
        else if (show_only_roughness)
        {
            lighting += roughness;
            continue;
        }
        else if (show_only_metalness)
        {
            lighting += metallic;
            continue;
        }
        else if (show_only_ao)
        {
            lighting += ao;
            continue;
        }

        Material m;
        m.albedo = albedo;
        m.roughness = roughness;
        m.metallic = metallic;
        const float3 Fdielectric = 0.04;
        m.f0 = lerp(Fdielectric, albedo, metallic);

        float3 V = normalize(viewPos - fragPos);
        float3 R = reflect(-V, normal);

        if (use_shadow)
        {
            float3 vL = fragPos - shadow_light_pos;
            float3 L = normalize(vL);
            float shadow = _sampleCubeShadowPCFDisc5(L, vL);
            lighting += CookTorrance_GGX(fragPos, normal, V, m, shadow_light_pos, 1, true) * shadow;
        }

        [unroll]
        for (int i = 0; i < LIGHT_COUNT; ++i)
        {
            lighting += CookTorrance_GGX(fragPos, normal, V, m, light_pos[i].rgb, light_color[i].rgb);
        }

        float3 ambient = 0;      
        if (use_IBL_diffuse)
        {
            // ambient lighting (we now use IBL as the ambient term)
            float3 kS = FresnelSchlickRoughness(max(dot(normal, V), 0.0), m.f0, roughness);
            float3 kD = 1.0 - kS;
            kD *= 1.0 - metallic;
            float3 irradiance = irradianceMap.Sample(g_sampler, normal).rgb;
            float3 diffuse = irradiance * albedo;
            ambient = (kD * diffuse) * ao;
        }

        if (use_IBL_specular)
        {
            float3 F = m.f0;
            if (use_f0_with_roughness)
                F = FresnelSchlickRoughness(max(dot(normal, V), 0.0), m.f0, roughness);
            // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
            uint width, height, levels;
            prefilterMap.GetDimensions(0, width, height, levels);
            float3 prefilteredColor = prefilterMap.SampleLevel(g_sampler, R, roughness * levels).rgb;
            float2 brdf = brdfLUT.Sample(brdf_sampler, float2(max(dot(normal, V), 0.0), roughness)).rg;
            float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
            ambient += specular * computeSpecOcclusion(max(dot(normal, V), 0.0), ao, roughness);
        }

        if (!use_IBL_diffuse && !use_IBL_specular)
            ambient = albedo * ao * 0.1;

        if (only_ambient)
            lighting = 0;

        lighting += ambient_power * ambient;
    }
    lighting /= SAMPLE_COUNT;

    return float4(lighting, 1.0);
}
