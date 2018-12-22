// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

//------------------------------------------------------------------------------
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
    bool use_filmic_hdr;
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
    else if (use_filmic_hdr)
    {
        color = ACESFitted(color);
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
