struct BindlessLocations
{
    uint index_buffer;
    uint vertex_buffer;
};

ConstantBuffer<BindlessLocations> constant_buffer: register(b0, space0);
StructuredBuffer<uint> structured_buffers_uint[] : register(t, space1);
StructuredBuffer<float3> structured_buffers_float3[] : register(t, space2);

float4 main(uint vertex_id : SV_VertexID) : SV_POSITION
{
    uint vertex_index = structured_buffers_uint[constant_buffer.index_buffer][vertex_id];
    return float4(structured_buffers_float3[constant_buffer.vertex_buffer][vertex_index], 1.0f);
}
