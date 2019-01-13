struct GeometryOutput
{
    float4 pos       : SV_POSITION;
    float3 fragPos   : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
    uint RTIndex     : SV_RenderTargetArrayIndex;
};

Texture2D alphaMap;

SamplerState g_sampler;

bool HasTexture(Texture2D _texture)
{
    uint width, height;
    _texture.GetDimensions(width, height);
    return width > 0 && height > 0;
}

void main(GeometryOutput input)
{
    if (HasTexture(alphaMap))
    {
        if (alphaMap.Sample(g_sampler, input.texCoord).r < 0.5)
            discard;
    }
}
