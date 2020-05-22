#pragma once
#include <cstdint>
#include <cassert>
#include <tuple>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <gli/format.hpp>

enum class ResourceState
{
    kCommon,
    kClear,
    kPresent,
    kRenderTarget,
    kDepthTarget,
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
    kTextureCubeArray,
    kRaytracingAccelerationStructure
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
    FillMode fill_mode = FillMode::kSolid;
    CullMode cull_mode = CullMode::kBack;
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
    bool depth_enable = true;
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
    ResourceType res_type;
    ResourceDimension dimension;
    uint32_t stride = 0;

    auto MakeTie() const
    {
        return std::tie(level, count, res_type, dimension, stride);
    }

    bool operator< (const ViewDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct BindKey
{
    ShaderType shader;
    ResourceType type;
    std::string name;

private:
    auto MakeTie() const
    {
        return std::tie(shader, type, name);
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

struct VertexInputDesc
{
    uint32_t slot = 0;
    std::string semantic_name;
    gli::format format = gli::format::FORMAT_UNDEFINED;
};

struct RenderTargetDesc
{
    uint32_t slot = 0;
    gli::format format = gli::format::FORMAT_UNDEFINED;
};

struct DepthStencilTargetDesc
{
    gli::format format = gli::format::FORMAT_UNDEFINED;
};

class Program;
class View;

struct GraphicsPipelineDesc
{
    std::shared_ptr<Program> program;
    std::vector<VertexInputDesc> input;
    std::vector<RenderTargetDesc> rtvs;
    DepthStencilTargetDesc dsv;
    DepthStencilDesc depth_desc;
    BlendDesc blend_desc;
    RasterizerDesc rasterizer_desc;
};

struct ComputePipelineDesc
{
    std::shared_ptr<Program> program;
};

struct BindingDesc
{
    ShaderType shader;
    ResourceType type;
    std::string name;
    std::shared_ptr<View> view;
};

struct BindKeyOld
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
    bool operator< (const BindKeyOld& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct ResourceBindingDesc
{
    ResourceDimension dimension;
};

enum class PipelineType
{
    kGraphics,
    kCompute,
};
