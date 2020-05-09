#pragma once

#include <cstdint>
#include <tuple>
#include <string>

enum class ResourceState
{
    kCommon,
    kClear,
    kPresent,
    kRenderTarget,
    kUnorderedAccess,
};

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
    kUnknown,
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
    KAccelerationStructure = 1 << 9,
};

enum ClearFlag
{
    kDepth = 1 << 0,
    kStencil = 1 << 1,
};

enum class FillMode
{
    kWireframe,
    kSolid
};

enum class CullMode
{
    kNone,
    kFront,
    kBack,
};

struct RasterizerDesc
{
    FillMode fill_mode;
    CullMode cull_mode;
    int32_t DepthBias = 0;
};


enum class Blend
{
    kZero,
    kSrcAlpha,
    kInvSrcAlpha,
};

enum class BlendOp
{
    kAdd,
};

struct BlendDesc
{
    bool blend_enable = false;
    Blend blend_src;
    Blend blend_dest;
    BlendOp blend_op;
    Blend blend_src_alpha;
    Blend blend_dest_apha;
    BlendOp blend_op_alpha;
};


enum class DepthComparison
{
    kLess,
    kLessEqual
};

struct DepthStencilDesc
{
    bool depth_enable;
    DepthComparison func = DepthComparison::kLess;
};

enum class ShaderType
{
    kUnknown,
    kVertex,
    kPixel,
    kCompute,
    kGeometry,
    kLibrary
};

struct BindKey
{
    size_t program_id;
    ShaderType shader_type;
    ResourceType res_type;
    uint32_t slot;

private:
    auto MakeTie() const
    {
        return std::tie(program_id, shader_type, res_type, slot);
    }

public:
    bool operator< (const BindKey& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};
