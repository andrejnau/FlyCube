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
TextureCube<float> cubeMapId;

SamplerState gTextureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

cbuffer Params
{
    float4x4 View[6];
    float4x4 Projection;
};

SamplerState g_sampler : register(s0);
SamplerComparisonState cubeShadowComparsionSampler : register(s1);

float sampleCubeShadowHPCF(float3 frag_pos, float3 light_pos)
{
    float3 vec = (frag_pos - light_pos);
    uint id = (uint) cubeMapId.Sample(gTextureSampler, vec).r;
    float4 target = float4(vec, 1.0);
    target = mul(target, View[id]);
    target = mul(target, Projection);
    target.xyz /= target.w;

    float depth = (target.z + 1.0) / 2.0;
    return cubeShadowMap.SampleCmpLevelZero(cubeShadowComparsionSampler, vec, depth).r;
}

float _vectorToDepth(float3 vec, float n, float f)
{
    float3 AbsVec = abs(vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    float NormZComp = (f + n) / (f - n) - (2 * f * n) / (f - n) / LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

float _sampleCubeShadowHPCF2(float3 frag_pos, float3 light_pos)
{
    float3 vL = frag_pos - light_pos;
    float3 L = normalize(vL);
    float sD = _vectorToDepth(vL, 0.0, 1.0);
    return cubeShadowMap.SampleCmpLevelZero(cubeShadowComparsionSampler, float3(L.xy, -L.z), sD).r;
}

float _sampleCubeShadowHPCF(float3 frag_pos, float3 light_pos)
{
    float3 shadowPos = frag_pos - light_pos;
    float3 shadowDistance = length(shadowPos);
    float3 shadowDir = normalize(shadowPos);

    // Doing the max of the components tells us 2 things: which cubemap face we're going to use,
    // and also what the projected distance is onto the major axis for that face.
    float projectedDistance = max(max(abs(shadowPos.x), abs(shadowPos.y)), abs(shadowPos.z));

    // Compute the project depth value that matches what would be stored in the depth buffer
    // for the current cube map face. Note that we use a reversed infinite projection.
    float nearClip = 1.0;
    float a = 0.0f;
    float b = nearClip;
    float z = projectedDistance * a + b;
    float dbDistance = (z / shadowDistance + 1.0) / 2.0;
    return cubeShadowMap.SampleCmpLevelZero(cubeShadowComparsionSampler, shadowDir, dbDistance).r;
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

    float3 hdrColor = float3(ambient + diffuse * _sampleCubeShadowHPCF(fragPos, lightPos) + specular);
    return pow(float4(hdrColor, 1.0), gamma4);
}
