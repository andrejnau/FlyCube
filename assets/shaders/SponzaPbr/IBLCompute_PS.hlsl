TextureCube<float> LightCubeShadowMap;

SamplerComparisonState LightCubeShadowComparsionSampler;

static const float PI = acos(-1.0);

#define LIGHT_COUNT 90

cbuffer Light
{
    bool use_light;
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
    float ambient_power;
    float light_power;
    bool use_normal_mapping;
    bool use_flip_normal_y;
    bool use_gloss_instead_of_roughness;
};

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

struct GeometryOutput
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
    uint RTIndex     : SV_RenderTargetArrayIndex;
};

Texture2D albedoMap;
Texture2D normalMap;
Texture2D glossMap;
Texture2D roughnessMap;
Texture2D metalnessMap;
Texture2D aoMap;
Texture2D alphaMap;

SamplerState g_sampler;

static const bool is_packed_normal = false;
static const bool is_sun_temple = false;
static const bool use_standard_channel_binding = true;

float4 getTexture(Texture2D _texture, SamplerState _sample, float2 _tex_coord, bool _need_gamma = false)
{
    float4 _color = _texture.Sample(_sample, _tex_coord);
    if (_need_gamma)
        _color = float4(pow(abs(_color.rgb), 2.2), _color.a);
    return _color;
}

struct PS_OUT
{
    float4 gAlbedo   : SV_Target0;
};

float3 CalcBumpedNormal(GeometryOutput input)
{
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 tbn = float3x3(T, B, N);
    float3 normal = normalMap.Sample(g_sampler, input.texCoord).rgb;
    if (use_flip_normal_y)
        normal.y = 1 - normal.y;
    normal = normalize(2.0 * normal - 1.0);
    if (is_packed_normal)
    {
        normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));
        normal = normalize(normal);
    }
    normal = normalize(mul(normal, tbn));
    return normal;
}

float4 main(GeometryOutput input) : SV_TARGET
{
    float3 lighting = 0;

    if (getTexture(alphaMap, g_sampler, input.texCoord).r < 0.5)
        discard;

    float3 fragPos = input.fragPos.xyz;
    float3 normal = CalcBumpedNormal(input);
    float3 albedo = getTexture(albedoMap, g_sampler, input.texCoord, true).rgb;
    float roughness = getTexture(roughnessMap, g_sampler, input.texCoord).r;
    float metallic = getTexture(metalnessMap, g_sampler, input.texCoord).r;

    float ao = pow(getTexture(aoMap, g_sampler, input.texCoord).r, 2.2);

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

    if (use_light)
    {
        [unroll]

        for (int i = 0; i < LIGHT_COUNT; ++i)
        {
            lighting += CookTorrance_GGX(fragPos, normal, V, m, light_pos[i].rgb, light_color[i].rgb);
        }
    }

    lighting += ambient_power * albedo * ao / 4;

    return float4(lighting, 1.0);
}
