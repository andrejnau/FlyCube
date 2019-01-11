cbuffer GSParams
{
    float4x4 View[6];
    float4x4 Projection;
};

struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
};

struct GeometryOutput
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
    uint RTIndex     : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle VS_OUTPUT input[3], inout TriangleStream<GeometryOutput> CubeMapStream)
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
                output.fragPos = input[v].fragPos;
                output.normal = input[v].normal;
                output.tangent = input[v].tangent;
                output.texCoord = input[v].texCoord;

                CubeMapStream.Append(output);
            }

            CubeMapStream.RestartStrip();
        }
    }
}
