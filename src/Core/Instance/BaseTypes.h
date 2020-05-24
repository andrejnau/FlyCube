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
    kClearColor,
    kClearDepth,
    kPresent,
    kRenderTarget,
    kDepthTarget,
    kPixelShaderResource,
    kNonPixelShaderResource,
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

enum class ViewType
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
        kAccelerationStructure = 1 << 9,
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

    auto MakeTie() const
    {
        return std::tie(fill_mode, cull_mode, DepthBias);
    }

    bool operator< (const RasterizerDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
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

    auto MakeTie() const
    {
        return std::tie(blend_enable, blend_src, blend_dest, blend_op, blend_src_alpha, blend_dest_apha, blend_op_alpha);
    }

    bool operator< (const BlendDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
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

    auto MakeTie() const
    {
        return std::tie(depth_enable, func);
    }

    bool operator< (const DepthStencilDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

enum class ShaderType
{
    kLibrary,
    kVertex,
    kPixel,
    kCompute,
    kGeometry,
    kUnknown
};

struct ViewDesc
{
    size_t level = 0;
    size_t count = static_cast<size_t>(-1);
    ViewType view_type;
    ResourceDimension dimension;
    uint32_t stride = 0;

    auto MakeTie() const
    {
        return std::tie(level, count, view_type, dimension, stride);
    }

    bool operator< (const ViewDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct BindKey
{
    ShaderType shader;
    ViewType type;
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

    auto MakeTie() const
    {
        return std::tie(slot, semantic_name, format);
    }

    bool operator< (const VertexInputDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct RenderTargetDesc
{
    uint32_t slot = 0;
    gli::format format = gli::format::FORMAT_UNDEFINED;

    auto MakeTie() const
    {
        return std::tie(slot, format);
    }

    bool operator< (const RenderTargetDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct DepthStencilTargetDesc
{
    gli::format format = gli::format::FORMAT_UNDEFINED;

    auto MakeTie() const
    {
        return std::tie(format);
    }

    bool operator< (const DepthStencilTargetDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
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

    auto MakeTie() const
    {
        return std::tie(program, input, rtvs, dsv, depth_desc, blend_desc, rasterizer_desc);
    }

    bool operator< (const GraphicsPipelineDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct ComputePipelineDesc
{
    std::shared_ptr<Program> program;

    auto MakeTie() const
    {
        return std::tie(program);
    }

    bool operator< (const ComputePipelineDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

using RayTracingPipelineDesc = ComputePipelineDesc;

struct BindingDesc
{
    ShaderType shader;
    ViewType type;
    std::string name;
    std::shared_ptr<View> view;

    auto MakeTie() const
    {
        return std::tie(shader, type, name, view);
    }

    bool operator< (const BindingDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct BindKeyOld
{
    size_t program_id;
    ShaderType shader_type;
    ViewType view_type;
    uint32_t slot;

    auto MakeTie() const
    {
        return std::tie(program_id, shader_type, view_type, slot);
    }

    bool operator< (const BindKeyOld& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct ResourceBindingDesc
{
    ResourceDimension dimension = ResourceDimension::kBuffer;
};

enum class PipelineType
{
    kGraphics,
    kCompute,
    kRayTracing,
};

class Resource;

struct BufferDesc
{
    std::shared_ptr<Resource> res;
    gli::format format = gli::format::FORMAT_UNDEFINED;
    uint32_t count = 0;
    uint32_t offset = 0;
};
