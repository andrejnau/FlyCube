cbuffer Params
{
    float4x4 View[6];
    float4x4 Projection;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
};

struct GeometryOutput
{
    float4 Position : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle VertexOutput input[3], inout TriangleStream<GeometryOutput> CubeMapStream)
{
	[unroll]
    for (int f = 0; f < 6; ++f)
    {
		{
            GeometryOutput output;

            output.RTIndex = f;

			[unroll]
            for (int v = 0; v < 3; ++v)
            {
                float4 worldPosition = input[v].Position;
                float4 viewPosition = mul(worldPosition, View[f]);
                output.Position = mul(viewPosition, Projection);

                CubeMapStream.Append(output);
            }

            CubeMapStream.RestartStrip();
        }
    }
}
