struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer cbv
{
    uint2 dim;
    bool use_tone_mapping;
    bool use_simple_hdr;
    bool use_simple_hdr2;
};

StructuredBuffer<float> lum;

Texture2D hdr_input;

static const float3x3 rgb2xyz = float3x3(
    0.4124564, 0.2126729, 0.0193339,
    0.3575761, 0.7151522, 0.1191920,
    0.1804375, 0.0721750, 0.9503041);

static const float3x3 xyz2rgb = float3x3(
    3.2404542, -0.9692660, 0.0556434,
    -1.5371385, 1.8760108, -0.2040259,
    -0.4985314, 0.0415560, 1.0572252);


float Exposure;
float White;

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 color = hdr_input.Load(uint3(dim * input.texCoord, 0)).rgb;

    if (use_simple_hdr)
    {
        color = color / (color + 1.0);
    }
    else if (use_simple_hdr2)
    {
        // Reinhard tonemapping operator.
        // see: "Photographic Tone Reproduction for Digital Images", eq. 4
        float luminance = dot(color * Exposure, float3(0.2126, 0.7152, 0.0722));
        float mappedLuminance = (luminance * (1.0 + luminance / (White * White))) / (1.0 + luminance);

        // Scale color by ratio of average luminances.
        color = (mappedLuminance / luminance) * color * Exposure;
    }
    else if (use_tone_mapping)
    {
        // Convert to XYZ
        float3 xyzCol = mul(color, rgb2xyz);

        // Convert to xyY
        float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
        float3 xyYCol = float3(xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);

        // Apply the tone mapping operation to the luminance (xyYCol.z or xyzCol.y)
        float L = (Exposure * xyYCol.z) / exp(lum[0] / (dim.x * dim.y));
        L = (L * (1 + L / (White * White))) / (1 + L);

        // Using the new luminance, convert back to XYZ
        xyzCol.x = (L * xyYCol.x) / (xyYCol.y);
        xyzCol.y = L;
        xyzCol.z = (L * (1 - xyYCol.x - xyYCol.y)) / xyYCol.y;

        color = mul(xyzCol, xyz2rgb);
    }

    float4 gamma4 = float4(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2, 1);
    return pow(float4(abs(color), 1.0), gamma4);
}
