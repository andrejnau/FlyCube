struct VsOutput {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

VsOutput main(uint vertex_id : SV_VertexID)
{
    VsOutput output;
	float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    output.uv = uv;
	return output;
}
