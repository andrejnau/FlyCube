struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 fragPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
};

Texture2D albedoMap;
Texture2D normalMap;
Texture2D roughnessMap;
Texture2D metalnessMap;
Texture2D aoMap;
Texture2D alphaMap;

SamplerState g_sampler;

#define LIGHT_COUNT 90

cbuffer light
{
    float4 light_color[LIGHT_COUNT];
    float4 light_pos[LIGHT_COUNT];
    float4 viewPos;
};

float4 getTexture(Texture2D _texture, SamplerState _sample, float2 _tex_coord, bool _need_gamma = false)
{
    float4 _color = _texture.Sample(_sample, _tex_coord);
    if (_need_gamma)
        _color = float4(pow(abs(_color.rgb), 2.2), _color.a);
    return _color;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

static const float PI = acos(-1.0);


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

bool is;
bool is2;

float3 NoramlMap(VS_OUTPUT input)
{
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 tbn = float3x3(T, B, N);
    float3 normal = normalMap.Sample(g_sampler, input.texCoord).rgb;
    normal = normalize(2.0 * normal - 1.0);
    return normalize(mul(normal, tbn));
}

struct Material_pbr
{
    float roughness;
    float metallic;
    float3 albedo;
    float3 f0;
};

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 CookTorrance_GGX(VS_OUTPUT input, float3 n, float3 v, Material_pbr m, int i)
{
    n = normalize(n);
    v = normalize(v);
    float3 l = normalize(light_pos[i] - input.fragPos);
    float3 h = normalize(v + l);

    float distance = length(light_pos[i] - input.fragPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = light_color[i] * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(n, h, m.roughness);
    float G = GeometrySmith(n, v, l, m.roughness);
    float3 F = fresnelSchlick(max(dot(h, v), 0.0), m.f0);

    float3 nominator = NDF * G * F;
    float denominator = 4 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.001;
    float3 specular = nominator / denominator;

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - m.metallic;

    float NdotL = max(dot(n, l), 0.0);

    return (kD * m.albedo / PI + specular) * radiance * NdotL;
}

bool HasTexture(Texture2D _texture)
{
    uint width, height;
    _texture.GetDimensions(width, height);
    return width > 0 && height > 0;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    if (HasTexture(alphaMap))
    {
        if (getTexture(alphaMap, g_sampler, input.texCoord).r < 0.5)
            discard;
    }

    float3 V = normalize(viewPos - input.fragPos);
    float3 N = NoramlMap(input);

    float3 albedo = getTexture(albedoMap, g_sampler, input.texCoord, true).rgb;
    float metallic = getTexture(metalnessMap, g_sampler, input.texCoord).r;
    float roughness = getTexture(roughnessMap, g_sampler, input.texCoord).r;

    float ao = 1;
    if (HasTexture(aoMap))
        ao = getTexture(aoMap, g_sampler, input.texCoord).r;

    Material_pbr m;
    m.roughness = roughness;
    m.albedo = albedo;
    m.metallic = metallic;
    float3 F0 = 0.04;
    m.f0 = lerp(F0, albedo, metallic);

    float3 _color = 0;
    [unroll]
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        _color += CookTorrance_GGX(input, N, V, m, i);
    }

    float3 ambient = 0.03 * albedo * ao;
    _color += ambient;

    _color = _color / (_color + 1.0);
    return float4(pow(abs(_color.rgb), 1 / 2.2), 1.0);
}
