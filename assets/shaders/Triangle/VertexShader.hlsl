struct VsOutput {
    float4 pos : SV_POSITION;
};

VsOutput main(float3 pos : POSITION)
{
    VsOutput output;
    output.pos = float4(pos, 1.0);
    return output;
}
