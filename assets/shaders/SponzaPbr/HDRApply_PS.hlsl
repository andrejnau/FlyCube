//--------------------------------------------------------------------------------
// thx https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
//--------------------------------------------------------------------------------
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

    return color;
}

//--------------------------------------------------------------------------------
// thx https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ShaderUtility.hlsli
//--------------------------------------------------------------------------------
// This assumes the default color gamut found in sRGB and REC709.  The color primaries determine these
// coefficients.  Note that this operates on linear values, not gamma space.
float RGBToLuminance(float3 x)
{
    return dot(x, float3(0.212671, 0.715160, 0.072169));        // Defined by sRGB/Rec.709 gamut
}

//--------------------------------------------------------------------------------

struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

StructuredBuffer<float> lum;

Texture2D hdr_input;

cbuffer HDRSetting
{
    bool use_reinhard_tone_operator;
    bool use_tone_mapping;
    bool use_white_balance;
    bool use_filmic_hdr;
    bool use_avg_lum;
    float exposure;
    float white;
};

float4 LinearTosRGBA(float3 color)
{
    return float4(pow(abs(color), 1 / 2.2), 1.0);
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    uint width, height;
    hdr_input.GetDimensions(width, height);
    float3 color = hdr_input.Load(uint3(input.texCoord * uint2(width, height), 0)).rgb;

    if (use_reinhard_tone_operator)
        return LinearTosRGBA(color / (1.0 + color));

    if (!use_tone_mapping)
        return LinearTosRGBA(color);

    static const float3x3 rgb2xyz = float3x3(
        0.4124564, 0.2126729, 0.0193339,
        0.3575761, 0.7151522, 0.1191920,
        0.1804375, 0.0721750, 0.9503041);

    static const float3x3 xyz2rgb = float3x3(
        3.2404542, -0.9692660, 0.0556434,
        -1.5371385, 1.8760108, -0.2040259,
        -0.4985314, 0.0415560, 1.0572252);

    // Convert to XYZ
    float3 xyzCol = mul(color, rgb2xyz);

    // Convert to xyY
    float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
    float3 xyYCol = float3(xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);

    // Apply the tone mapping operation to the luminance (xyYCol.z or xyzCol.y)
    float luminance = (exposure * xyYCol.z);
    if (use_avg_lum)
        luminance /= exp(lum[0]);
    if (use_white_balance)
    {
        // see: "Photographic Tone Reproduction for Digital Images", eq. 4
        luminance = (luminance * (1.0 + luminance / (white * white))) / (1.0 + luminance);
    }
    else
    {
        // see: "Photographic Tone Reproduction for Digital Images", eq. 3
        luminance = luminance / (1.0 + luminance);
    }

    // Using the new luminance, convert back to XYZ
    xyzCol.x = (luminance * xyYCol.x) / (xyYCol.y);
    xyzCol.y = luminance;
    xyzCol.z = (luminance * (1 - xyYCol.x - xyYCol.y)) / xyYCol.y;

    color = mul(xyzCol, xyz2rgb);

    if (use_filmic_hdr)
        color = ACESFitted(color);

    return LinearTosRGBA(color);
}
