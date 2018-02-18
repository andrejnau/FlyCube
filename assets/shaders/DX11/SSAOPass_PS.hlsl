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
Texture2D noiseTexture;

#define KERNEL_SIZE 64

cbuffer SSAOBuffer
{
    float4x4 projection;
    float4 samples[KERNEL_SIZE];
    int width;
    int height;
    bool is;
};

float4 getTexture(Texture2DMS<float4> _texture, float2 _tex_coord, int ss_index)
{
    float3 gbufferDim;
    _texture.GetDimensions(gbufferDim.x, gbufferDim.y, gbufferDim.z);
    float2 texcoord = _tex_coord * float2(gbufferDim.xy);
    float4 _color = _texture.Load(texcoord, ss_index);
    return _color;
}

float4 getTexture(Texture2D _texture, float2 _tex_coord, int ss_index = 0)
{
    uint2 dim;
    _texture.GetDimensions(dim.x, dim.y);
    float4 _color = _texture.Load(uint3(dim * _tex_coord, 0));
    return _color;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float radius = 0.4;
    float bias = 0.025;
    const float2 noiseScale = float2(width / 4.0, height / 4.0);

    float occlusions = 0;
    [unroll]
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float3 fragPos = getTexture(gPosition, input.texCoord, i).xyz;
        float3 normal = normalize(getTexture(gNormal, input.texCoord, i).xyz);
        float3 tangent = normalize(getTexture(noiseTexture, ((input.texCoord * noiseScale) % 4) / 4.0, i)).xyz;
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        float3 bitangent = normalize(cross(tangent, normal));
        float3x3 TBN = float3x3(tangent, bitangent, normal);

        float occlusion = 0.0;
        for (int i = 0; i < KERNEL_SIZE; ++i)
        {
            // get sample position
            float3 sample_pos = mul(samples[i].xyz, TBN); // from tangent to view-space
            sample_pos = fragPos + sample_pos * radius;

            // project sample position (to sample texture) (to get position on screen/texture)
            float4 offset = float4(sample_pos, 1.0);
            offset = mul(offset, projection); // from view to clip-space

            offset.xyz /= offset.w; // perspective divide
            offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
            offset.y = 1.0 - offset.y;

             // get sample depth
            float sampleDepth = getTexture(gPosition, offset.xy, i).z;

             // range check & accumulate
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
            occlusion += (sampleDepth >= sample_pos.z + bias ? 1.0 : 0.0) * rangeCheck;
        }
        occlusions += 1.0 - (occlusion / float(KERNEL_SIZE));
    }
    occlusions /= SAMPLE_COUNT;

    return float4(occlusions, 0.0, 0.0, 0.0);
}
