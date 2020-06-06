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
    kGenericRead,
    kClearColor,
    kClearDepth,
    kPresent,
    kRenderTarget,
    kDepthTarget,
    kPixelShaderResource,
    kNonPixelShaderResource,
    kUnorderedAccess,
    kCopyDest,
    kCopySource,
    kVertexAndConstantBuffer,
    kIndexBuffer,
    kRaytracingAccelerationStructure
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
    kShaderResource,
    kUnorderedAccess,
    kConstantBuffer,
    kSampler,
    kRenderTarget,
    kDepthStencil
};

enum class ResourceType
{
    kUnknown,
    kBuffer,
    kTexture,
    kSampler,
    kBottomLevelAS,
    kTopLevelAS,
};

namespace BindFlag
{
    enum
    {
        kRenderTarget = 1 << 1,
        kDepthStencil = 1 << 2,
        kShaderResource = 1 << 3,
        kUnorderedAccess = 1 << 4,
        kConstantBuffer = 1 << 5,
        kIndexBuffer = 1 << 6,
        kVertexBuffer = 1 << 7,
        kAccelerationStructure = 1 << 8,
        kRayTracing = 1 << 9,
        kCopyDest = 1 << 10,
        kCopySource = 1 << 11,
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
    kUnknown,
    kVertex,
    kPixel,
    kCompute,
    kGeometry,
    kLibrary,
};

struct LazyViewDesc
{
    size_t level = 0;
    size_t count = static_cast<size_t>(-1);

    auto MakeTie() const
    {
        return std::tie(level, count);
    }

    bool operator< (const LazyViewDesc& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct ViewDesc : public LazyViewDesc
{
    ViewType view_type;
    ResourceDimension dimension;
    uint32_t stride = 0;
    uint32_t offset = 0;
    bool bindless = false;

    auto MakeTie() const
    {
        return std::tie(level, count, view_type, dimension, stride, offset, bindless);
    }

    bool operator< (const ViewDesc& oth) const
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
    uint32_t stride = 0;

    auto MakeTie() const
    {
        return std::tie(slot, semantic_name, format, stride);
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

struct BindKey
{
    ShaderType shader_type = ShaderType::kUnknown;
    ViewType view_type = ViewType::kUnknown;
    uint32_t slot = 0;
    uint32_t space = 0;

    auto MakeTie() const
    {
        return std::tie(shader_type, view_type, slot, space);
    }

    bool operator< (const BindKey& oth) const
    {
        return MakeTie() < oth.MakeTie();
    }
};

struct BindingDesc
{
    BindKey bind_key;
    std::shared_ptr<View> view;

    auto MakeTie() const
    {
        return std::tie(bind_key, view);
    }

    bool operator< (const BindingDesc& oth) const
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

enum class MemoryType
{
    kDefault,
    kUpload
};

struct TextureOffset
{
    int32_t x;
    int32_t y;
    int32_t z;
};

struct TextureExtent3D
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct BufferToTextureCopyRegion
{
    uint64_t buffer_offset;
    uint32_t buffer_row_pitch;
    uint32_t texture_subresource;
    TextureOffset texture_offset;
    TextureExtent3D texture_extent;
};

struct BufferCopyRegion
{
    uint64_t src_offset;
    uint64_t dst_offset;
    uint64_t num_bytes;
};

struct GeometryInstance
{
    glm::mat3x4 transform;
    uint32_t instance_id : 24;
    uint32_t instance_mask : 8;
    uint32_t instance_offset : 24;
    uint32_t flags : 8;
    uint64_t acceleration_structure_handle;
};

static_assert(sizeof(GeometryInstance) == 64);

struct ResourceBarrierDesc
{
    std::shared_ptr<Resource> resource;
    ResourceState state_before;
    ResourceState state_after;
    uint32_t base_mip_level = 0;
    uint32_t level_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t layer_count = 1;
};

struct ResourceLazyViewDesc
{
    std::shared_ptr<Resource> resource;
    LazyViewDesc view_desc;
};

class CommandListBox;

class DeferredView
{
public:
    virtual ResourceLazyViewDesc GetView(CommandListBox& command_list) = 0;
};
