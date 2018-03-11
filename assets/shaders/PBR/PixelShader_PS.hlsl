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
Texture2D metalnessMap;
Texture2D aoMap;
Texture2D alphaMap;

SamplerState g_sampler : register(s0);

float4 getTexture(Texture2D _texture, SamplerState _sample, float2 _tex_coord, bool _need_gamma = false)
{
    float4 _color = _texture.Sample(_sample, _tex_coord);
    if (_need_gamma)
        _color = float4(pow(abs(_color.rgb), 2.2), _color.a);
    return _color;
}

float4 BaseLighting(VS_OUTPUT input)
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
    float diff = saturate(dot(input.normal, lightDir));
    float3 diffuse_base = getTexture(diffuseMap, g_sampler, input.texCoord, true).rgb;
    float3 diffuse = diffuse_base * diff;

    // Specular
    float3 viewDir = normalize(input.viewPos - input.fragPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    float reflectivity = getTexture(glossMap, g_sampler, input.texCoord).r;
    float spec = pow(saturate(dot(viewDir, reflectDir)), reflectivity * 1024.0);
    float3 specular_base = 0.5;
    float3 specular = specular_base * spec;

    float4 _color = float4(ambient + diffuse + specular, 1.0);
    return float4(pow(abs(_color.rgb), 1 / 2.2), _color.a);
}

float GGX_PartialGeometry(float cosThetaN, float alpha)
{
    float cosTheta_sqr = saturate(cosThetaN*cosThetaN);
    float tan2 = (1 - cosTheta_sqr) / cosTheta_sqr;
    float GP = 2 / (1 + sqrt(1 + alpha * alpha * tan2));
    return GP;
}
static const float PI = acos(-1.0);

float GGX_Distribution(float cosThetaNH, float alpha) {
    float alpha2 = alpha * alpha;
    float NH_sqr = saturate(cosThetaNH * cosThetaNH);
    float den = NH_sqr * alpha2 + (1.0 - NH_sqr);
    return alpha2 / (PI * den * den);
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
    float3 albedo;
    float3 f0;
};

float3 FresnelSchlick(float3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(1.0 - saturate(cosTheta), 5.0);
}

float3 CookTorrance_GGX(float3 n, float3 l, float3 v, Material_pbr m) {
    n = normalize(n);
    v = normalize(v);
    l = normalize(l);
    float3 h = normalize(v + l);
    //precompute dots
    float NL = dot(n, l);
    if (NL <= 0.0) return 0.0;
    float NV = dot(n, v);
    if (NV <= 0.0) return 0.0;
    float NH = dot(n, h);
    float HV = dot(h, v);

    //precompute roughness square
    float roug_sqr = m.roughness*m.roughness;

    //calc coefficients
    float G = GGX_PartialGeometry(NV, roug_sqr) * GGX_PartialGeometry(NL, roug_sqr);
    float D = GGX_Distribution(NH, roug_sqr);
    float3 F = FresnelSchlick(m.f0, HV);

    //mix
    float3 specK = G * D*F*0.25 / (NV + 0.001);
    float3 diffK = saturate(1.0 - F);
    return max(0.0, m.albedo*diffK*NL / PI + 5 * specK);
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

    float3 V = normalize(input.viewPos - input.fragPos);
    float3 L = normalize(input.lightPos - input.fragPos);
    float3 N = NoramlMap(input);

    float3 albedo = getTexture(diffuseMap, g_sampler, input.texCoord, true).rgb;
    float metallic = getTexture(metalnessMap, g_sampler, input.texCoord).r;
    float roughness = getTexture(glossMap, g_sampler, input.texCoord).r;

    float ao = 1;
    if (HasTexture(aoMap))
        ao = getTexture(aoMap, g_sampler, input.texCoord).r;

    Material_pbr m;
    m.roughness = roughness;
    m.albedo = albedo;
    float3 F0 = 0;
    m.f0 = lerp(F0, albedo, metallic); 
   
    float3 _color = CookTorrance_GGX(N, L, V, m) + 0.2 * albedo * ao;
     _color = _color / (_color + 1);

    return float4(pow(abs(_color.rgb), 1 / 2.2), 1.0);
}
