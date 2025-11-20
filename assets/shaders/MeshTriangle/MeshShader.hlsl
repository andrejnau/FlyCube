struct MeshOutput {
    float4 pos   : SV_Position;
    float4 color : COLOR0;
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void main(out indices uint3 triangles[1], out vertices MeshOutput vertices[3])
{
    SetMeshOutputCounts(3, 1);

    triangles[0] = uint3(0, 1, 2);

    vertices[0].pos = float4(-0.5, -0.5, 0.0, 1.0);
    vertices[0].color = float4(1.0, 0.0, 0.0, 1.0);

    vertices[1].pos = float4(0.5, -0.5, 0.0, 1.0);
    vertices[1].color = float4(0.0, 1.0, 0.0, 1.0);

    vertices[2].pos = float4(0.0, 0.5, 0.0, 1.0);
    vertices[2].color = float4(0.0, 0.0, 1.0, 1.0);
}
