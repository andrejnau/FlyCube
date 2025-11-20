RaytracingAccelerationStructure geometry : register(t0, space0);
RWTexture2D<float4> result_texture : register(u1, space0);

static const uint kRayFlags =
    RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;

[numthreads(8, 8, 1)]
void main(uint2 thread_id : SV_DispatchThreadID)
{
    uint2 texture_size;
    result_texture.GetDimensions(texture_size.x, texture_size.y);
    if (any(thread_id >= texture_size)) {
        return;
    }

    float2 uv = (thread_id + 0.5) / texture_size;
    float2 ndc = uv * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = float3(0.0, 0.0, -1.0);
    ray.Direction = normalize(float3(ndc.x, -ndc.y, 1.0));
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    RayQuery<kRayFlags> ray_query;
    ray_query.TraceRayInline(geometry, kRayFlags, 0xFF, ray);
    ray_query.Proceed();

    float3 color = float3(0.0, 0.2, 0.4);
    if (ray_query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        if (ray_query.CommittedInstanceID() == 0) {
            color = float3(1.0, 0.0, 0.0);
        } else {
            color = float3(0.0, 1.0, 0.0);
        }
    }
    result_texture[thread_id] = float4(color, 1.0);
}
