//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12MeshShaders

cbuffer Constants
{
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
};

cbuffer MeshInfo
{
    uint IndexCount;
    uint IndexOffset;
};

struct VertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    uint   MeshletIndex : COLOR0;
};

Buffer<float3>            Position;
StructuredBuffer<float3>  Normal;
Buffer<uint>              VertexIndices;

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    VertexOut vout;
    vout.PositionVS = mul(float4(Position[vertexIndex], 1), WorldView).xyz;
    vout.PositionHS = mul(float4(Position[vertexIndex], 1), WorldViewProj);
    vout.Normal = mul(float4(Normal[vertexIndex], 0), World).xyz;
    vout.MeshletIndex = meshletIndex;

    return vout;
}

#define MAX_OUTPUT_PRIMITIVES 10

[NumThreads(MAX_OUTPUT_PRIMITIVES * 3, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[MAX_OUTPUT_PRIMITIVES],
    out vertices VertexOut verts[MAX_OUTPUT_PRIMITIVES * 3]
)
{
    uint PrimitivesCount = MAX_OUTPUT_PRIMITIVES;
    if (PrimitivesCount > IndexCount - 3 * gid * MAX_OUTPUT_PRIMITIVES)
    {
        PrimitivesCount = IndexCount - 3 * gid * MAX_OUTPUT_PRIMITIVES;
    }
    uint VertexCount = 3 * PrimitivesCount;
    SetMeshOutputCounts(VertexCount, PrimitivesCount);
    if (gtid < PrimitivesCount)
    {
        tris[gtid] = uint3(gtid * 3, gtid * 3 + 1, gtid * 3 + 2);
    }
    if (gtid < VertexCount)
    {
        uint vertexIndex = VertexIndices[IndexOffset + 3 * gid * MAX_OUTPUT_PRIMITIVES + gtid];
        verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    }
}
