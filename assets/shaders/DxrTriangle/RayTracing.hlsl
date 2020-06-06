RaytracingAccelerationStructure geometry;
RWTexture2D<float4> result;

struct RayPayload
{
    float3 color;
};

[shader("raygeneration")]
void ray_gen()
{
    float2 d = ((float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy)) * 2.f - 1.f);

    RayDesc ray;
    ray.Origin = float3(0, 0, -1);
    ray.Direction = normalize(float3(d.x, -d.y, 1));
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    RayPayload payload;
    TraceRay(geometry, 0, 0xFF, 0, 0, 0, ray, payload);
    result[DispatchRaysIndex().xy] = float4(payload.color, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = float3(0.0, 0.2, 0.4);
}

[shader("closesthit")]
void closest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(1, 0, 0);
}
