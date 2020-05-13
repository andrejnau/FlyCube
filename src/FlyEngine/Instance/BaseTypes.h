#pragma once
#include <cstdint>
#include <cassert>
#include <tuple>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <gli/gli.hpp>

enum class ResourceState
{
    kCommon,
    kClear,
    kPresent,
    kRenderTarget,
    kUnorderedAccess,
};

enum class ResourceDimension
{
    kBuffer,
    kTexture1D,
    kTexture1DArray,
    kTexture2D,
    kTexture2DArray,
    kTexture2DMS,
    kTexture2DMSArray,
    kTexture3D,
    kTextureCube,
    kTextureCubeArray
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

namespace BindFlag
{
    enum
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
}

namespace ClearFlag
{
    enum
    {
        kDepth = 1 << 0,
        kStencil = 1 << 1,
    };
}

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

struct ViewDesc
{
    size_t level = 0;
    size_t count = static_cast<size_t>(-1);
    ResourceDimension dimension;
    uint32_t stride = 0;
    ResourceType res_type;
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

struct ShaderDesc
{
    std::string shader_path;
    std::string entrypoint;
    ShaderType type;
    std::map<std::string, std::string> define;

    ShaderDesc(const std::string& shader_path, const std::string& entrypoint, ShaderType type)
        : shader_path(shader_path)
        , entrypoint(entrypoint)
        , type(type)
    {
    }
};

class Program;
class View;

struct PipelineStateDesc
{
    std::shared_ptr<Program> program;
    std::vector<std::shared_ptr<View>> rtvs;
    std::shared_ptr<View> dsv;
};
