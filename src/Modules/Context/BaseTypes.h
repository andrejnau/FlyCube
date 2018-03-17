#pragma once

enum class SamplerFilter
{
    kAnisotropic,
    kMinMagMipLinear,
    kComparisonMinMagMipLinear,
};

enum class SamplerTextureAddressMode
{
    kWrap,
    kClamp
};

enum class SamplerComparisonFunc
{
    kNever,
    kAlways,
    kLess
};

struct SamplerDesc
{
    SamplerFilter filter;
    SamplerTextureAddressMode mode;
    SamplerComparisonFunc func;
};

enum class ResourceType
{
    kSrv,
    kUav,
    kCbv,
    kSampler,
    kRtv,
    kDsv
};
