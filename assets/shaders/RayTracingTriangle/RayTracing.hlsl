RaytracingAccelerationStructure geometry : register(t0, space0);
RWTexture2D<float4> result_texture : register(u1, space0);

static const uint kRayFlags =
    RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;

struct RayPayload {
    float3 color;
};

[shader("raygeneration")]
void ray_gen()
{
    float2 uv = (DispatchRaysIndex().xy + 0.5) / DispatchRaysDimensions().xy;
    float2 ndc = uv * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = float3(0.0, 0.0, -1.0);
    ray.Direction = normalize(float3(ndc.x, -ndc.y, 1.0));
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    RayPayload payload;
    TraceRay(geometry, kRayFlags, 0xFF, 0, 0, 0, ray, payload);
    result_texture[DispatchRaysIndex().xy] = float4(payload.color, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = float3(0.0, 0.2, 0.4);
}
