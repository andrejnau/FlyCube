struct RayPayload
{
    float3 color;
};

[shader("closesthit")]
void closest_red(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(1, 0, 0);
}

[shader("closesthit")]
void closest_green(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    CallShader(0, payload);
}
