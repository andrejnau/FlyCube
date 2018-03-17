#pragma once

#include <cstdint>

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

enum BindFlag
{
    kRtv = 1 << 1,
    kDsv = 1 << 2,
    kSrv = 1 << 3,
    kUav = 1 << 4,
    kCbv = 1 << 5,
    kIbv = 1 << 6,
    kVbv = 1 << 7,
    kSampler = 1 << 8,
};
