#include "BoneTransform.hlsli"

StructuredBuffer<float3> in_position;
StructuredBuffer<float3> in_normal;
StructuredBuffer<float3> in_tangent;
StructuredBuffer<uint> bones_offset;
StructuredBuffer<uint> bones_count;

RWStructuredBuffer<float3> out_position;
RWStructuredBuffer<float3> out_normal;
RWStructuredBuffer<float3> out_tangent;

StructuredBuffer<uint> index_buffer;

cbuffer cbv
{
    uint IndexCount;
    uint StartIndexLocation;
    uint BaseVertexLocation;
};

[numthreads(256, 1, 1)]
void main(uint3 threadId : SV_DispatchthreadId, uint groupId : SV_GroupIndex, uint3 dispatchId : SV_GroupID)
{
    if (threadId.x >= IndexCount)
        return;
    uint vertex_id = index_buffer[StartIndexLocation + threadId.x] + BaseVertexLocation;
    float4x4 transform = GetBoneTransform(bones_count[vertex_id], bones_offset[vertex_id]);
    out_position[vertex_id] = mul(float4(in_position[vertex_id], 1.0), transform).xyz;
    out_normal[vertex_id] = mul(in_normal[vertex_id], transform);
    out_tangent[vertex_id] = mul(in_tangent[vertex_id], transform);
}
