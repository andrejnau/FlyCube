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
TextureCube<float> cubeShadowMap;

cbuffer Params
{
    float4x4 View[6];
    float4x4 Projection;
};

SamplerState g_sampler : register(s0);
SamplerComparisonState cubeShadowComparsionSampler : register(s1);

uint convert_xyz_to_cube_uv(float3 v)
{
    float x = v.x;
    float y = v.y;
    float z = v.z;

    float absX = abs(x);
    float absY = abs(y);
    float absZ = abs(z);

    int isXPositive = x > 0 ? 1 : 0;
    int isYPositive = y > 0 ? 1 : 0;
    int isZPositive = z > 0 ? 1 : 0;

    uint res = 0;

    if (isXPositive && absX >= absY && absX >= absZ)
    {
        res = 0;
    }
    else if (!isXPositive && absX >= absY && absX >= absZ)
    {
        res = 1;
    }
    else if (isYPositive && absY >= absX && absY >= absZ)
    {
        res = 2;
    }
    else if (!isYPositive && absY >= absX && absY >= absZ)
    {
        res = 3;
    }
    else if (isZPositive && absZ >= absX && absZ >= absY)
    {
        res = 4;
    }
    else if (!isZPositive && absZ >= absX && absZ >= absY)
    {
        res = 5;
    }
    else
    {
        discard;
    }

    return res;
}

float sampleCubeShadow(float3 frag_pos, float3 light_pos, uint id)
{
    float3 vec = normalize(frag_pos - light_pos);
    float4 target = float4(frag_pos, 1.0);
    target = mul(target, mul(View[id], Projection));
    //target.xyz /= target.w;
    float depth = (target.z + 1.0) * 0.5;
    return cubeShadowMap.SampleCmpLevelZero(cubeShadowComparsionSampler, vec, depth).r;
}

float sampleCubeShadowHPCF(float3 frag_pos, float3 light_pos)
{
    uint id = convert_xyz_to_cube_uv(frag_pos - light_pos);
    return sampleCubeShadow(frag_pos, light_pos, id);
}

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
    float3 lightPos;
    float3 viewPos;
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

    float3 hdrColor = float3(ambient + diffuse * sampleCubeShadowHPCF(fragPos, lightPos) + specular);
    return pow(float4(hdrColor, 1.0), gamma4);
}
