struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 fragPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangent: TANGENT;
};

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specularMap : register(t2);
Texture2D glossMap : register(t3);
Texture2D ambientMap : register(t4);

SamplerState g_sampler : register(s0);

cbuffer TexturesEnables : register(b0)
{
    int has_diffuseMap;
    int has_normalMap;
    int has_specularMap;
    int has_glossMap;
    int has_ambientMap;
};

cbuffer Material : register(b1)
{
    namespace material
    {
        float3 ambient;
        float3 diffuse;
        float3 specular;
        float shininess;
    }
};

cbuffer Light : register(b2)
{
    namespace light
    {
        float3 ambient;
        float3 diffuse;
        float3 specular;
    }
};

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

struct PS_OUT
{   float4 gPosition : SV_Target0;
    float3 gNormal   : SV_Target1;
    float3 gAmbient  : SV_Target2; 
    float3 gDiffuse  : SV_Target3;
    float4 gSpecular : SV_Target4;
    float3 gGloss    : SV_Target5;
};

float3 CalcBumpedNormal(VS_OUTPUT input)
{
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(T, N));
    float3x3 tbn = float3x3(T, B, N);
    float3 normal = normalMap.Sample(g_sampler, input.texCoord).rgb;
    normal = normalize(2.0 * normal - 1.0);
    normal = normalize(mul(normal, tbn));
    return normal;
}

PS_OUT main(VS_OUTPUT input)
{
    PS_OUT output;
    output.gPosition = float4(input.fragPos.xyz, input.pos.z);

    if (has_normalMap)
        output.gNormal = CalcBumpedNormal(input);
    else
        output.gNormal = normalize(input.normal);

    output.gAmbient = light::ambient;
    if (has_ambientMap)
        output.gAmbient *= getTexture(ambientMap, g_sampler, input.texCoord, true).rgb;
    else
        output.gAmbient *= material::ambient;

    output.gDiffuse = light::diffuse;
    if (has_diffuseMap)
        output.gDiffuse *= getTexture(diffuseMap, g_sampler, input.texCoord, true).rgb;
    else
        output.gDiffuse *= material::diffuse;

    output.gSpecular.rgb = light::specular;
    output.gSpecular.a = material::shininess;
    if (has_specularMap)
        output.gSpecular.rgb *= getTexture(specularMap, g_sampler, input.texCoord, true).rgb;
    else
        output.gSpecular.rgb *= material::specular;

    if (has_glossMap)
        output.gGloss = getTexture(glossMap, g_sampler, input.texCoord, true).rgb;
    else
        output.gGloss = float3(1.0, 1.0, 1.0);
    return output;
}
