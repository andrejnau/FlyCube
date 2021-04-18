struct RayPayload
{
    float3 color;
};

[shader("callable")]
void callable(inout RayPayload payload)
{
    payload.color = float3(0, 1, 0);
}
