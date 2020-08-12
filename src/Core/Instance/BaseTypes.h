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
    kUnknown,
    kUndefined,
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
    kRaytracingAccelerationStructure,
    kShadingRateSource,
};

enum class ResourceDimension
{
    kUnknown,
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
        kShadingRateSource = 1 << 12,
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
    int32_t depth_bias = 0;

    auto MakeTie() const
    {
        return std::tie(fill_mode, cull_mode, depth_bias);
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
};

struct ViewDesc : public LazyViewDesc
{
    ViewType view_type = ViewType::kUnknown;
    ResourceDimension dimension = ResourceDimension::kUnknown;
    uint32_t stride = 0;
    uint32_t offset = 0;
    bool bindless = false;

    auto MakeTie() const
    {
        return std::tie(level, count, view_type, dimension, stride, offset, bindless);
    }
};

struct ShaderDesc
{
    std::string shader_path;
    std::string entrypoint;
    ShaderType type;
    std::string model;
    std::map<std::string, std::string> define;

    ShaderDesc(const std::string& shader_path, const std::string& entrypoint, ShaderType type, const std::string& model)
        : shader_path(shader_path)
        , entrypoint(entrypoint)
        , type(type)
        , model(model)
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
};

enum class RenderPassLoadOp
{
    kLoad,
    kClear,
    kDontCare,
};

enum class RenderPassStoreOp
{
    kStore = 0,
    kDontCare,
};

struct RenderPassColorDesc
{
    gli::format format;
    RenderPassLoadOp load_op = RenderPassLoadOp::kLoad;
    RenderPassStoreOp store_op = RenderPassStoreOp::kStore;

    auto MakeTie() const
    {
        return std::tie(format, load_op, store_op);
    }
};

struct RenderPassDepthStencilDesc
{
    gli::format format;
    RenderPassLoadOp depth_load_op = RenderPassLoadOp::kLoad;
    RenderPassStoreOp depth_store_op = RenderPassStoreOp::kStore;
    RenderPassLoadOp stencil_load_op = RenderPassLoadOp::kLoad;
    RenderPassStoreOp stencil_store_op = RenderPassStoreOp::kStore;

    auto MakeTie() const
    {
        return std::tie(format, depth_load_op, depth_store_op, stencil_load_op, stencil_store_op);
    }
};

struct RenderPassDesc
{
    std::vector<RenderPassColorDesc> colors;
    RenderPassDepthStencilDesc depth_stencil;
    uint32_t sample_count = 1;

    auto MakeTie() const
    {
        return std::tie(colors, depth_stencil, sample_count);
    }
};

class Program;
class View;
class RenderPass;

struct GraphicsPipelineDesc
{
    std::shared_ptr<Program> program;
    std::vector<VertexInputDesc> input;
    std::shared_ptr<RenderPass> render_pass;
    DepthStencilDesc depth_desc;
    BlendDesc blend_desc;
    RasterizerDesc rasterizer_desc;

    auto MakeTie() const
    {
        return std::tie(program, input, render_pass, depth_desc, blend_desc, rasterizer_desc);
    }
};

struct ComputePipelineDesc
{
    std::shared_ptr<Program> program;

    auto MakeTie() const
    {
        return std::tie(program);
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
};

struct BindingDesc
{
    BindKey bind_key;
    std::shared_ptr<View> view;

    auto MakeTie() const
    {
        return std::tie(bind_key, view);
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

enum class RaytracingInstanceFlags : uint32_t
{
    kNone = 0x0,
    kTriangleCullDisable = 0x1,
    kTriangleFrontCounterclockwise = 0x2,
    kForceOpaque = 0x4,
    kForceNonOpaque = 0x8
};

enum class RaytracingGeometryFlags
{
    kNone,
    kOpaque,
    kNoDuplicateAnyHitInvocation
};

struct RaytracingGeometryDesc
{
    BufferDesc vertex;
    BufferDesc index;
    RaytracingGeometryFlags flags = RaytracingGeometryFlags::kNone;
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
    uint32_t texture_mip_level;
    uint32_t texture_array_layer;
    TextureOffset texture_offset;
    TextureExtent3D texture_extent;
};

struct TextureCopyRegion
{
    TextureExtent3D extent;
    uint32_t src_mip_level;
    uint32_t src_array_layer;
    TextureOffset src_offset;
    uint32_t dst_mip_level;
    uint32_t dst_array_layer;
    TextureOffset dst_offset;
};

struct BufferCopyRegion
{
    uint64_t src_offset;
    uint64_t dst_offset;
    uint64_t num_bytes;
};

struct RaytracingGeometryInstance
{
    glm::mat3x4 transform;
    uint32_t instance_id : 24;
    uint32_t instance_mask : 8;
    uint32_t instance_offset : 24;
    RaytracingInstanceFlags flags : 8;
    uint64_t acceleration_structure_handle;
};

static_assert(sizeof(RaytracingGeometryInstance) == 64);

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

class CommandListBox;
struct ResourceLazyViewDesc;

class DeferredView
{
public:
    virtual std::shared_ptr<ResourceLazyViewDesc> GetView(CommandListBox& command_list) = 0;
    virtual void OnDestroy(ResourceLazyViewDesc& view_desc) = 0;
};

struct ResourceLazyViewDesc
{
    ResourceLazyViewDesc(DeferredView& deferred_view, const std::shared_ptr<Resource>& resource)
        : m_deferred_view(deferred_view)
        , resource(resource)
    {
    }

    ~ResourceLazyViewDesc()
    {
        m_deferred_view.OnDestroy(*this);
    }

    std::shared_ptr<Resource> resource;
    LazyViewDesc view_desc;
    DeferredView& m_deferred_view;
};


enum class ShadingRate : uint8_t
{
    k1x1 = 0,
    k1x2 = 0x1,
    k2x1 = 0x4,
    k2x2 = 0x5,
    k2x4 = 0x6,
    k4x2 = 0x9,
    k4x4 = 0xa,
};

enum class ShadingRateCombiner
{
   kPassthrough = 0,
   kOverride = 1,
   kMin = 2,
   kMax = 3,
   kSum = 4,
};

struct RaytracingASPrebuildInfo
{
    uint64_t build_scratch_data_size;
};

enum class CommandListType
{
    kGraphics,
    kCompute,
    kCopy,
};

struct ClearDesc
{
    std::vector<glm::vec4> colors;
    float depth;
};

enum class CopyAccelerationStructureMode
{
    kClone,
    kCompact,
};

namespace enum_class
{
    enum BuildAccelerationStructureFlags
    {
        kNone = 0,
        kAllowUpdate = 1 << 0,
        kAllowCompaction = 1 << 1,
        kPreferFastTrace = 1 << 2,
        kPreferFastBuild = 1 << 3,
        kMinimizeMemory = 1 << 4,
    };
}

using BuildAccelerationStructureFlags = enum_class::BuildAccelerationStructureFlags;

template<typename T> 
auto operator< (const T& l, const T& r) -> std::enable_if_t<std::is_same_v<decltype(l.MakeTie() < r.MakeTie()), bool>, bool>
{
    return l.MakeTie() < r.MakeTie();
}
