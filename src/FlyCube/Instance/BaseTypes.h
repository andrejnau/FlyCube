#pragma once
#include "EnumUtils.h"

#include <gli/format.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

class BindingSetLayout;
class Program;
class RenderPass;
class Resource;
class View;

namespace enum_class {
enum ResourceState : uint32_t {
    kUnknown = 0,
    kCommon = 1 << 0,
    kVertexAndConstantBuffer = 1 << 1,
    kIndexBuffer = 1 << 2,
    kRenderTarget = 1 << 3,
    kUnorderedAccess = 1 << 4,
    kDepthStencilWrite = 1 << 5,
    kDepthStencilRead = 1 << 6,
    kNonPixelShaderResource = 1 << 7,
    kPixelShaderResource = 1 << 8,
    kIndirectArgument = 1 << 9,
    kCopyDest = 1 << 10,
    kCopySource = 1 << 11,
    kRaytracingAccelerationStructure = 1 << 12,
    kShadingRateSource = 1 << 13,
    kPresent = 1 << 14,
    kGenericRead = ResourceState::kVertexAndConstantBuffer | ResourceState::kIndexBuffer | ResourceState::kCopySource |
                   ResourceState::kNonPixelShaderResource | ResourceState::kPixelShaderResource |
                   ResourceState::kIndirectArgument,
    kUndefined = 1 << 15,
};
}

using ResourceState = enum_class::ResourceState;
ENABLE_BITMASK_OPERATORS(ResourceState);

enum class ViewDimension {
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
};

enum class SamplerFilter {
    kAnisotropic,
    kMinMagMipLinear,
    kComparisonMinMagMipLinear,
};

enum class SamplerTextureAddressMode { kWrap, kClamp };

enum class SamplerComparisonFunc { kNever, kAlways, kLess };

struct SamplerDesc {
    SamplerFilter filter;
    SamplerTextureAddressMode mode;
    SamplerComparisonFunc func;
};

enum class ViewType {
    kUnknown,
    kConstantBuffer,
    kSampler,
    kTexture,
    kRWTexture,
    kBuffer,
    kRWBuffer,
    kStructuredBuffer,
    kRWStructuredBuffer,
    kAccelerationStructure,
    kShadingRateSource,
    kRenderTarget,
    kDepthStencil
};

enum class ShaderBlobType {
    kDXIL,
    kSPIRV,
};

enum class ResourceType {
    kUnknown,
    kBuffer,
    kTexture,
    kSampler,
    kAccelerationStructure,
};

enum class TextureType {
    k1D,
    k2D,
    k3D,
};

namespace BindFlag {
enum {
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
    kShaderTable = 1 << 13,
    kIndirectBuffer = 1 << 14
};
}

enum class FillMode { kWireframe, kSolid };

enum class CullMode {
    kNone,
    kFront,
    kBack,
};

struct RasterizerDesc {
    FillMode fill_mode = FillMode::kSolid;
    CullMode cull_mode = CullMode::kNone;
    int32_t depth_bias = 0;

    auto MakeTie() const
    {
        return std::tie(fill_mode, cull_mode, depth_bias);
    }
};

enum class Blend {
    kZero,
    kSrcAlpha,
    kInvSrcAlpha,
};

enum class BlendOp {
    kAdd,
};

struct BlendDesc {
    bool blend_enable = false;
    Blend blend_src;
    Blend blend_dest;
    BlendOp blend_op;
    Blend blend_src_alpha;
    Blend blend_dest_apha;
    BlendOp blend_op_alpha;

    auto MakeTie() const
    {
        return std::tie(blend_enable, blend_src, blend_dest, blend_op, blend_src_alpha, blend_dest_apha,
                        blend_op_alpha);
    }
};

enum class ComparisonFunc { kNever, kLess, kEqual, kLessEqual, kGreater, kNotEqual, kGreaterEqual, kAlways };

enum class StencilOp { kKeep, kZero, kReplace, kIncrSat, kDecrSat, kInvert, kIncr, kDecr };

struct StencilOpDesc {
    StencilOp fail_op = StencilOp::kKeep;
    StencilOp depth_fail_op = StencilOp::kKeep;
    StencilOp pass_op = StencilOp::kKeep;
    ComparisonFunc func = ComparisonFunc::kAlways;

    auto MakeTie() const
    {
        return std::tie(fail_op, depth_fail_op, pass_op, func);
    }
};

struct DepthStencilDesc {
    bool depth_test_enable = true;
    ComparisonFunc depth_func = ComparisonFunc::kLess;
    bool depth_write_enable = true;
    bool depth_bounds_test_enable = false;
    bool stencil_enable = false;
    uint8_t stencil_read_mask = 0xff;
    uint8_t stencil_write_mask = 0xff;
    StencilOpDesc front_face = {};
    StencilOpDesc back_face = {};

    auto MakeTie() const
    {
        return std::tie(depth_test_enable, depth_func, depth_write_enable, depth_bounds_test_enable, stencil_enable,
                        stencil_read_mask, stencil_write_mask, front_face, back_face);
    }
};

enum class ShaderType {
    kUnknown,
    kVertex,
    kPixel,
    kCompute,
    kGeometry,
    kAmplification,
    kMesh,
    kLibrary,
};

struct ViewDesc {
    ViewType view_type = ViewType::kUnknown;
    ViewDimension dimension = ViewDimension::kUnknown;
    uint32_t base_mip_level = 0;
    uint32_t level_count = static_cast<uint32_t>(-1);
    uint32_t base_array_layer = 0;
    uint32_t layer_count = static_cast<uint32_t>(-1);
    uint32_t plane_slice = 0;
    uint64_t offset = 0;
    uint32_t structure_stride = 0;
    uint64_t buffer_size = static_cast<uint64_t>(-1);
    gli::format buffer_format = gli::format::FORMAT_UNDEFINED;
    bool bindless = false;

    auto MakeTie() const
    {
        return std::tie(view_type, dimension, base_mip_level, level_count, base_array_layer, layer_count, plane_slice,
                        offset, structure_stride, buffer_size, buffer_format, bindless);
    }
};

struct ShaderDesc {
    std::string shader_path;
    std::string entrypoint;
    ShaderType type;
    std::string model;
    std::map<std::string, std::string> define;

    ShaderDesc() = default;

    ShaderDesc(const std::string& shader_path, const std::string& entrypoint, ShaderType type, const std::string& model)
        : shader_path(shader_path)
        , entrypoint(entrypoint)
        , type(type)
        , model(model)
    {
    }
};

struct InputLayoutDesc {
    uint32_t slot = 0;
    std::string semantic_name;
    gli::format format = gli::format::FORMAT_UNDEFINED;
    uint32_t stride = 0;

    auto MakeTie() const
    {
        return std::tie(slot, semantic_name, format, stride);
    }
};

enum class RenderPassLoadOp {
    kLoad,
    kClear,
    kDontCare,
};

enum class RenderPassStoreOp {
    kStore = 0,
    kDontCare,
};

struct RenderPassColorDesc {
    gli::format format = gli::format::FORMAT_UNDEFINED;
    RenderPassLoadOp load_op = RenderPassLoadOp::kLoad;
    RenderPassStoreOp store_op = RenderPassStoreOp::kStore;

    auto MakeTie() const
    {
        return std::tie(format, load_op, store_op);
    }
};

struct RenderPassDepthStencilDesc {
    gli::format format = gli::format::FORMAT_UNDEFINED;
    RenderPassLoadOp depth_load_op = RenderPassLoadOp::kLoad;
    RenderPassStoreOp depth_store_op = RenderPassStoreOp::kStore;
    RenderPassLoadOp stencil_load_op = RenderPassLoadOp::kLoad;
    RenderPassStoreOp stencil_store_op = RenderPassStoreOp::kStore;

    auto MakeTie() const
    {
        return std::tie(format, depth_load_op, depth_store_op, stencil_load_op, stencil_store_op);
    }
};

struct RenderPassDesc {
    std::vector<RenderPassColorDesc> colors;
    RenderPassDepthStencilDesc depth_stencil;
    gli::format shading_rate_format = gli::format::FORMAT_UNDEFINED;
    uint32_t sample_count = 1;

    auto MakeTie() const
    {
        return std::tie(colors, depth_stencil, shading_rate_format, sample_count);
    }
};

struct FramebufferDesc {
    std::shared_ptr<RenderPass> render_pass;
    uint32_t width;
    uint32_t height;
    std::vector<std::shared_ptr<View>> colors;
    std::shared_ptr<View> depth_stencil;
    std::shared_ptr<View> shading_rate_image;

    auto MakeTie() const
    {
        return std::tie(render_pass, width, height, colors, depth_stencil, shading_rate_image);
    }
};

struct GraphicsPipelineDesc {
    std::shared_ptr<Program> program;
    std::shared_ptr<BindingSetLayout> layout;
    std::vector<InputLayoutDesc> input;
    std::shared_ptr<RenderPass> render_pass;
    DepthStencilDesc depth_stencil_desc;
    BlendDesc blend_desc;
    RasterizerDesc rasterizer_desc;

    auto MakeTie() const
    {
        return std::tie(program, layout, input, render_pass, depth_stencil_desc, blend_desc, rasterizer_desc);
    }
};

struct ComputePipelineDesc {
    std::shared_ptr<Program> program;
    std::shared_ptr<BindingSetLayout> layout;

    auto MakeTie() const
    {
        return std::tie(program, layout);
    }
};

enum class RayTracingShaderGroupType {
    kGeneral,
    kTrianglesHitGroup,
    kProceduralHitGroup,
};

struct RayTracingShaderGroup {
    RayTracingShaderGroupType type = RayTracingShaderGroupType::kGeneral;
    uint64_t general = 0;
    uint64_t closest_hit = 0;
    uint64_t any_hit = 0;
    uint64_t intersection = 0;

    auto MakeTie() const
    {
        return std::tie(type, general, closest_hit, any_hit, intersection);
    }
};

struct RayTracingPipelineDesc {
    std::shared_ptr<Program> program;
    std::shared_ptr<BindingSetLayout> layout;
    std::vector<RayTracingShaderGroup> groups;

    auto MakeTie() const
    {
        return std::tie(program, layout, groups);
    }
};

struct RayTracingShaderTable {
    std::shared_ptr<Resource> resource;
    uint64_t offset;
    uint64_t size;
    uint64_t stride;
};

struct RayTracingShaderTables {
    RayTracingShaderTable raygen;
    RayTracingShaderTable miss;
    RayTracingShaderTable hit;
    RayTracingShaderTable callable;
};

struct BindKey {
    ShaderType shader_type = ShaderType::kUnknown;
    ViewType view_type = ViewType::kUnknown;
    uint32_t slot = 0;
    uint32_t space = 0;
    uint32_t count = 1;
    uint32_t remapped_slot = ~0;

    uint32_t GetRemappedSlot() const
    {
        if (remapped_slot == ~0) {
            return slot;
        }
        return remapped_slot;
    }

    auto MakeTie() const
    {
        return std::tie(shader_type, view_type, slot, space, count);
    }
};

struct BindingDesc {
    BindKey bind_key;
    std::shared_ptr<View> view;

    auto MakeTie() const
    {
        return std::tie(bind_key, view);
    }
};

enum class ReturnType {
    kUnknown,
    kFloat,
    kUint,
    kInt,
    kDouble,
};

struct ResourceBindingDesc {
    std::string name;
    ViewType type;
    uint32_t slot;
    uint32_t space;
    uint32_t count;
    ViewDimension dimension;
    ReturnType return_type;
    uint32_t structure_stride;
};

enum class PipelineType {
    kGraphics,
    kCompute,
    kRayTracing,
};

struct BufferDesc {
    std::shared_ptr<Resource> res;
    gli::format format = gli::format::FORMAT_UNDEFINED;
    uint32_t count = 0;
    uint32_t offset = 0;
};

enum class RaytracingInstanceFlags : uint32_t {
    kNone = 0x0,
    kTriangleCullDisable = 0x1,
    kTriangleFrontCounterclockwise = 0x2,
    kForceOpaque = 0x4,
    kForceNonOpaque = 0x8
};

enum class RaytracingGeometryFlags { kNone, kOpaque, kNoDuplicateAnyHitInvocation };

struct RaytracingGeometryDesc {
    BufferDesc vertex;
    BufferDesc index;
    RaytracingGeometryFlags flags = RaytracingGeometryFlags::kNone;
};

enum class MemoryType { kDefault, kUpload, kReadback };

struct TextureOffset {
    int32_t x;
    int32_t y;
    int32_t z;
};

struct TextureExtent3D {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct BufferToTextureCopyRegion {
    uint64_t buffer_offset;
    uint32_t buffer_row_pitch;
    uint32_t texture_mip_level;
    uint32_t texture_array_layer;
    TextureOffset texture_offset;
    TextureExtent3D texture_extent;
};

struct TextureCopyRegion {
    TextureExtent3D extent;
    uint32_t src_mip_level;
    uint32_t src_array_layer;
    TextureOffset src_offset;
    uint32_t dst_mip_level;
    uint32_t dst_array_layer;
    TextureOffset dst_offset;
};

struct BufferCopyRegion {
    uint64_t src_offset;
    uint64_t dst_offset;
    uint64_t num_bytes;
};

struct RaytracingGeometryInstance {
    glm::mat3x4 transform;
    uint32_t instance_id : 24;
    uint32_t instance_mask : 8;
    uint32_t instance_offset : 24;
    RaytracingInstanceFlags flags : 8;
    uint64_t acceleration_structure_handle;
};

static_assert(sizeof(RaytracingGeometryInstance) == 64);

struct ResourceBarrierDesc {
    std::shared_ptr<Resource> resource;
    ResourceState state_before;
    ResourceState state_after;
    uint32_t base_mip_level = 0;
    uint32_t level_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t layer_count = 1;
};

enum class ShadingRate : uint8_t {
    k1x1 = 0,
    k1x2 = 0x1,
    k2x1 = 0x4,
    k2x2 = 0x5,
    k2x4 = 0x6,
    k4x2 = 0x9,
    k4x4 = 0xa,
};

enum class ShadingRateCombiner {
    kPassthrough = 0,
    kOverride = 1,
    kMin = 2,
    kMax = 3,
    kSum = 4,
};

struct RaytracingASPrebuildInfo {
    uint64_t acceleration_structure_size = 0;
    uint64_t build_scratch_data_size = 0;
    uint64_t update_scratch_data_size = 0;
};

enum class AccelerationStructureType {
    kTopLevel,
    kBottomLevel,
};

enum class CommandListType {
    kGraphics,
    kCompute,
    kCopy,
};

struct ClearDesc {
    std::vector<glm::vec4> colors;
    float depth = 1.0f;
    uint8_t stencil = 0;
};

enum class CopyAccelerationStructureMode {
    kClone,
    kCompact,
};

namespace enum_class {
enum BuildAccelerationStructureFlags {
    kNone = 0,
    kAllowUpdate = 1 << 0,
    kAllowCompaction = 1 << 1,
    kPreferFastTrace = 1 << 2,
    kPreferFastBuild = 1 << 3,
    kMinimizeMemory = 1 << 4,
};
}

using BuildAccelerationStructureFlags = enum_class::BuildAccelerationStructureFlags;
ENABLE_BITMASK_OPERATORS(BuildAccelerationStructureFlags);

struct DrawIndirectCommand {
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t first_vertex;
    uint32_t first_instance;
};

struct DrawIndexedIndirectCommand {
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t vertex_offset;
    uint32_t first_instance;
};

struct DispatchIndirectCommand {
    uint32_t thread_group_count_x;
    uint32_t thread_group_count_y;
    uint32_t thread_group_count_z;
};

using IndirectCountType = uint32_t;

constexpr uint64_t kAccelerationStructureAlignment = 256;

enum class QueryHeapType { kAccelerationStructureCompactedSize };

template <typename T>
auto operator<(const T& l, const T& r)
    -> std::enable_if_t<std::is_same_v<decltype(l.MakeTie() < r.MakeTie()), bool>, bool>
{
    return l.MakeTie() < r.MakeTie();
}
