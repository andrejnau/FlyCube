#include "3rdparty/lygia/generative/voronoi.hlsl"

struct ConstantLayout {
    float time;
};

ConstantBuffer<ConstantLayout> constant_buffer : register(b0, space0);
RWTexture2D<float4> result_texture : register(u1, space0);

[numthreads(8, 8, 1)]
void main(uint2 thread_id : SV_DispatchThreadID)
{
    uint2 texture_size;
    result_texture.GetDimensions(texture_size.x, texture_size.y);
    if (any(thread_id >= texture_size)) {
        return;
    }

    result_texture[thread_id] = float4(voronoi(float3(thread_id / 16.0, constant_buffer.time)), 1.0);
}
