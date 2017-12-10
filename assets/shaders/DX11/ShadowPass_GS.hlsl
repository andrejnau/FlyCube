cbuffer Params
{
    float4x4 View[6];
    float4x4 Projection;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord  : TEXCOORD;
};

struct GeometryOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord  : TEXCOORD;
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
                float4 worldPosition = input[v].pos;
                float4 viewPosition = mul(worldPosition, View[f]);
                output.pos = mul(viewPosition, Projection);
                output.texCoord = input[v].texCoord;

                CubeMapStream.Append(output);
            }

            CubeMapStream.RestartStrip();
        }
    }
}
