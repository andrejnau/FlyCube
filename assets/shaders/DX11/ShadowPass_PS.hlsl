cbuffer Params
{
    float3 light_pos;
};

struct GeometryOutput
{
    float4 Position : SV_POSITION;
    float4 PositionW : POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

float4 main(GeometryOutput input) : SV_Target
{
    float depthValue = length(input.PositionW.xyz - light_pos);
    float4 color = float4(depthValue, (float)input.RTIndex, 1.0, 1.0);
	return color;
}
