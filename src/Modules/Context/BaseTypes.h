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

enum BindFlag
{
    kRtv = 1 << 1,
    kDsv = 1 << 2,
    kSrv = 1 << 3,
    kUav = 1 << 4,
    kCbv = 1 << 5,
    kIbv = 1 << 6,
    kVbv = 1 << 7,
};

enum class ResourceType
{
    kSrv,
    kUav,
    kCbv,
    kSampler
};
