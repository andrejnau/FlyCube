struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 fragPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
    float3 bitangent: BITANGENT;
    float3 lightPos : POSITION_LIGHT;
    float3 viewPos : POSITION_VIEW;
};

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specularMap : register(t2);
Texture2D glossMap : register(t3);
Texture2D ambientMap : register(t4);

SamplerState g_sampler : register(s0);

#define USE_CAMMA_RT
#define USE_CAMMA_TEX

float4 getTexture(Texture2D _texture, SamplerState _sample, float2 _tex_coord, bool _need_gamma = false)
{
    float4 _color = _texture.Sample(_sample, _tex_coord);
#ifdef USE_CAMMA_TEX
    if (_need_gamma)
        _color = float4(pow(abs(_color.rgb), 2.2), _color.a);
#endif
    return _color;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 tbn = float3x3(T, B, N);
    float3 normal = normalMap.Sample(g_sampler, input.texCoord).rgb;
    normal = normalize(2.0 * normal - 1.0);
    input.normal = normalize(mul(normal, tbn));

    // Ambient
    float3 ambient = getTexture(ambientMap, g_sampler, input.texCoord, true).rgb;

    // Diffuse
    float3 lightDir = normalize(input.lightPos - input.fragPos);
    float diff = saturate(dot(lightDir, input.normal));
    float3 diffuse_base = getTexture(diffuseMap, g_sampler, input.texCoord, true).rgb;
    float3 diffuse = diffuse_base * diff;

    // Specular
    float3 viewDir = normalize(input.viewPos - input.fragPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    float reflectivity = getTexture(glossMap, g_sampler, input.texCoord).r;
    float spec = pow(saturate(dot(input.normal, reflectDir)), reflectivity * 1024.0);
#if 0
    float3 specular_base = getTexture(specularMap, g_sampler, input.texCoord).rgb;
#else
    float3 specular_base = 0.5;
#endif
    float3 specular = specular_base * spec;

    float4 _color = float4(ambient + diffuse + specular, 1.0);
#ifdef USE_CAMMA_RT
    return float4(pow(abs(_color.rgb), 1 / 2.2), _color.a);
#else
    return _color;
#endif
}