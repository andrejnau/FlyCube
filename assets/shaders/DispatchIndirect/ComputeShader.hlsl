#include "../../../3rdparty/lygia/generative/voronoi.hlsl"

struct ConstantLayout
{
    float time;
};

ConstantBuffer<ConstantLayout> cbv;

RWTexture2D<float4> uav;

[numthreads(8, 8, 1)]
void main(uint2 thread_id : SV_DispatchThreadID)
{
    uav[thread_id] = float4(voronoi(float3(thread_id / 16.0, cbv.time)), 1.0);
}
