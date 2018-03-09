struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
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
TEXTURE_TYPE gAmbient;
TEXTURE_TYPE gDiffuse;
TEXTURE_TYPE gSpecular;
Texture2D gSSAO;
TextureCube<float> LightCubeShadowMap;

cbuffer ShadowParams
{
    float s_near;
    float s_far;
    float s_size;
    bool use_shadow;
    bool use_occlusion;
};

SamplerState g_sampler : register(s0);
SamplerComparisonState LightCubeShadowComparsionSampler : register(s1);

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

#define USE_CAMMA_RT
#define USE_CAMMA_TEX

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
    bool blinn;
};

float3 CalcLighting(float3 fragPos, float3 normal, float3 ambient, float3 diffuse, float3 specular, float shininess, float occlusion)
{
    float3 lightDir = normalize(lightPos - fragPos);
    float3 viewDir = normalize(viewPos - fragPos);

    float diff = saturate(dot(normal, lightDir));

    float spec = 0;
    if (blinn)
        spec = pow(saturate(dot(normalize(lightDir + viewDir), normal)), shininess);
    else
        spec = pow(saturate(dot(viewDir, reflect(-lightDir, normal))), shininess);

    float shadow = 1.0; 
    if (use_shadow)
    {
        float3 vL = fragPos - lightPos;
        float3 L = normalize(vL);
        shadow = _sampleCubeShadowPCFDisc5(L, vL);
    }

    return ambient * occlusion + shadow * (diff * diffuse + spec * specular);
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 lighting = 0;
    [unroll]
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float3 fragPos = getTexture(gPosition, input.texCoord, i).rgb;
        float3 normal = getTexture(gNormal, input.texCoord, i).rgb;
        float3 ambient = getTexture(gAmbient, input.texCoord, i).rgb;
        float3 diffuse = getTexture(gDiffuse, input.texCoord, i).rgb;
        float3 specular = getTexture(gSpecular, input.texCoord, i).rgb;
        float shininess = getTexture(gSpecular, input.texCoord, i).a;
        float occlusion = gSSAO.Sample(g_sampler, input.texCoord).r;
        occlusion = pow(occlusion, 2.2);
        if (!use_occlusion)
            occlusion = 1.0;
        lighting += CalcLighting(fragPos, normal, ambient, diffuse, specular, shininess, occlusion);
    }
    lighting /= SAMPLE_COUNT;

    return float4(lighting, 1.0);
}
