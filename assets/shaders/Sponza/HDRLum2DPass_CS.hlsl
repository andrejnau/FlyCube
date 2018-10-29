Texture2D<float4> data;
RWStructuredBuffer<float> result;

cbuffer cbv
{
    uint2 dispatchSize;
};

#define numthread 32

groupshared float accum[numthread * numthread];

[numthreads(numthread, numthread, 1)]
void main(uint3 threadId : SV_DispatchthreadId, uint groupId : SV_GroupIndex, uint3 dispatchId : SV_GroupID)
{
    float3 color = data.Load(uint3(threadId.xy, 0)).rgb;
    float lum = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float log_lum = log(lum + 0.00001f);
    accum[groupId] = log_lum;

    for (uint block_size = numthread * numthread / 2; block_size >= 1; block_size >>= 1)
    {
        GroupMemoryBarrierWithGroupSync();
        if (groupId < block_size)
        {
            accum[groupId] += accum[groupId + block_size];
        }
    }

    if (groupId == 0)
    {
        result[dispatchId.x * dispatchSize.y + dispatchId.y] = accum[0];
    }
}
