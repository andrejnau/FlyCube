struct GeometryOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord  : TEXCOORD;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

Texture2D alphaMap;

SamplerState g_sampler : register(s0);

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
