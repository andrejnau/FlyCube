//--------------------------------------------------------------------------------
// copy of https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/05-ao/Data/Tutorial05/hlslUtils.hlsli
//--------------------------------------------------------------------------------

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;

    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;

    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

//--------------------------------------------------------------------------------
// base on https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/05-ao/Data/Tutorial05/aoTracing.rt.hlsl
//--------------------------------------------------------------------------------

[[vk::binding(0)]]
cbuffer Settings : register(b0)
{
    float ao_radius;
    uint frame_index;
    uint num_rays;
};

#ifndef SAMPLE_COUNT
#define SAMPLE_COUNT 1
#endif

#if SAMPLE_COUNT > 1
#define TEXTURE_TYPE Texture2DMS<float4>
#else
#define TEXTURE_TYPE Texture2D
#endif

[[vk::binding(1)]]
TEXTURE_TYPE gPosition : register(t0);

[[vk::binding(2)]]
TEXTURE_TYPE gNormal : register(t1);

[[vk::binding(3)]]
RaytracingAccelerationStructure geometry : register(t2);

[[vk::binding(4)]]
RWTexture2D<float4> result : register(u0);

struct RayPayload
{
    float value;
};

float4 GetValue(Texture2DMS<float4> tex, uint2 tex_coord, uint ss_index)
{
    return tex.Load(tex_coord, ss_index);
}

float4 GetValue(Texture2D tex, uint2 tex_coord, uint ss_index = 0)
{
    return tex[tex_coord];
}

float ShootAmbientOcclusionRay(float3 orig, float3 dir)
{
    RayPayload rayPayload = { 0.0f };
    RayDesc rayAO;
    rayAO.Origin = orig;
    rayAO.Direction = dir;
    rayAO.TMin = 1.0e-2f;
    rayAO.TMax = ao_radius;

    TraceRay(geometry, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 1, 0, rayAO, rayPayload);

    return rayPayload.value.x;
}

[shader("raygeneration")]
void ray_gen()
{
    uint2 launch_index = DispatchRaysIndex().xy;
    uint2 launch_dim = DispatchRaysDimensions().xy;
    uint rand_seed = initRand(launch_index.x + launch_index.y * launch_dim.x, frame_index, 16);
    uint kernel_per_sample = max(1, num_rays / SAMPLE_COUNT);
    float ao = 0.0;
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float4 position = GetValue(gPosition, launch_index, i);
        float3 normal = GetValue(gNormal, launch_index, i).xyz;
        if (position.w == 0.0)
            continue;
        for (uint j = i * kernel_per_sample; j < (i + 1) * kernel_per_sample; ++j)
        {
            float3 dir = getCosHemisphereSample(rand_seed, normal);
            ao += 1 - ShootAmbientOcclusionRay(position.xyz, dir);
        }
    }
    ao = 1 - ao / (kernel_per_sample * SAMPLE_COUNT);
    result[launch_index] = float4(ao, ao, ao, 0.0);
}

[shader("miss")]
void miss(inout RayPayload rayData)
{
    rayData.value = 1.0f;
}

[shader("closesthit")]
void closest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.value = float3(1, 0, 0);
}
