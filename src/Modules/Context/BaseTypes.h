#pragma once

enum class SamplerFilter
{
    kAnisotropic,
    kComparisonMinMagMipLinear
};

enum class SamplerTextureAddressMode
{
    kWrap,
    kClamp
};

enum class SamplerComparisonFunc
{
    kNever,
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
