struct RayPayload
{
    float3 color;
};

[shader("closesthit")]
void closest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(1, 0, 0);
}
