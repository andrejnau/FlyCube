Texture2D<float4> data;
RWStructuredBuffer<float> result;

cbuffer cb
{
    uint2 dispatchSize;
};

// thx https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ShaderUtility.hlsli
// This assumes the default color gamut found in sRGB and REC709.  The color primaries determine these
// coefficients.  Note that this operates on linear values, not gamma space.
float RGBToLuminance(float3 x)
{
    return dot(x, float3(0.212671, 0.715160, 0.072169));		// Defined by sRGB/Rec.709 gamut
}

#define numthread 32

groupshared float accum[numthread * numthread];

[numthreads(numthread, numthread, 1)]
void main(uint3 threadId : SV_DispatchthreadId, uint groupId : SV_GroupIndex, uint3 dispatchId : SV_GroupID)
{
    float3 color = data.Load(uint3(threadId.xy, 0)).rgb;
    float lum = RGBToLuminance(color);

    uint width, height;
    data.GetDimensions(width, height);

    accum[groupId] = log(max(lum, 0.00001f)) / (width * height);
    GroupMemoryBarrierWithGroupSync();
    [unroll]
    for (uint block_size = numthread * numthread / 2; block_size >= 1; block_size >>= 1)
    {
        if (groupId < block_size)
        {
            accum[groupId] += accum[groupId + block_size];
        }
        GroupMemoryBarrierWithGroupSync();
    }
    if (groupId == 0)
    {
        result[dispatchId.x * dispatchSize.y + dispatchId.y] = accum[0];
    }
}
