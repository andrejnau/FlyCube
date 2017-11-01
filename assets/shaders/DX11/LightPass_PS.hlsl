struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D gPosition;
Texture2D gNormal;
Texture2D gAmbient;
Texture2D gDiffuse;
Texture2D gSpecular;

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

cbuffer ConstantBuffer
{
    float4 lightPos;
    float4 viewPos;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 gamma4 = float4(1.0/2.2, 1.0 / 2.2, 1.0 / 2.2, 1);

    float3 fragPos = getTexture(gPosition, g_sampler, input.texCoord, false).rgb;
    float3 normal = getTexture(gNormal, g_sampler, input.texCoord, false).rgb;
    float3 ambient = getTexture(gAmbient, g_sampler, input.texCoord, false).rgb;
    float3 diffuse = getTexture(gDiffuse, g_sampler, input.texCoord, false).rgb;
    float3 specular_base = getTexture(gSpecular, g_sampler, input.texCoord, false).rgb;
    float shininess = getTexture(gSpecular, g_sampler, input.texCoord, false).a;

    float3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    diffuse *= diff;

    float3 viewDir = normalize(viewPos - fragPos);
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(saturate(dot(viewDir, reflectDir)), shininess);
    float3 specular = specular_base * spec;

    float3 hdrColor = float3(ambient + diffuse + specular);
    return pow(float4(hdrColor, 1.0), gamma4);
}
