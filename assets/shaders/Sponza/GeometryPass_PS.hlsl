struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
};

Texture2D normalMap;
Texture2D alphaMap;
Texture2D ambientMap;
Texture2D diffuseMap;
Texture2D specularMap;
Texture2D shininessMap;

SamplerState g_sampler;

cbuffer Material
{
    bool use_normal_mapping;
};

cbuffer Light
{
    float3 light_ambient;
    float3 light_diffuse;
    float3 light_specular;
};

float4 getTexture(Texture2D _texture, SamplerState _sample, float2 _tex_coord, bool _need_gamma = false)
{
    float4 _color = _texture.Sample(_sample, _tex_coord);
    if (_need_gamma)
        _color = float4(pow(abs(_color.rgb), 2.2), _color.a);
    return _color;
}

struct PS_OUT
{
    float4 gPosition : SV_Target0;
    float4 gNormal   : SV_Target1;
    float4 gAmbient  : SV_Target2;
    float4 gDiffuse  : SV_Target3;
    float4 gSpecular : SV_Target4;
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
    if (getTexture(alphaMap, g_sampler, input.texCoord).r < 0.5)
        discard;

    PS_OUT output;
    output.gPosition = float4(input.fragPos.xyz, 1);

    if (use_normal_mapping)
        output.gNormal.rgb = CalcBumpedNormal(input);
    else
        output.gNormal.rgb = normalize(input.normal);
    output.gNormal.a = 1.0;

    output.gAmbient.rgb = light_ambient;
    output.gAmbient.rgb *= getTexture(ambientMap, g_sampler, input.texCoord, true).rgb;
    output.gAmbient.a = 1.0;

    output.gDiffuse.rgb = light_diffuse;
    output.gDiffuse.rgb *= getTexture(diffuseMap, g_sampler, input.texCoord, true).rgb;
    output.gDiffuse.a = 1.0;

    output.gSpecular.rgb = light_specular;
    output.gSpecular.rgb *= getTexture(specularMap, g_sampler, input.texCoord, true).rgb;
    output.gSpecular.a = getTexture(shininessMap, g_sampler, input.texCoord, false).r;

    return output;
}
