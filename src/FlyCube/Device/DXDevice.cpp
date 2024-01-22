#include "Device/DXDevice.h"

#include "Adapter/DXAdapter.h"
#include "BindingSet/DXBindingSet.h"
#include "BindingSetLayout/DXBindingSetLayout.h"
#include "CommandList/DXCommandList.h"
#include "CommandQueue/DXCommandQueue.h"
#include "Fence/DXFence.h"
#include "Framebuffer/DXFramebuffer.h"
#include "Memory/DXMemory.h"
#include "Pipeline/DXComputePipeline.h"
#include "Pipeline/DXGraphicsPipeline.h"
#include "Pipeline/DXRayTracingPipeline.h"
#include "Program/ProgramBase.h"
#include "QueryHeap/DXRayTracingQueryHeap.h"
#include "RenderPass/DXRenderPass.h"
#include "Shader/ShaderBase.h"
#include "Swapchain/DXSwapchain.h"
#include "Utilities/DXGIFormatHelper.h"
#include "Utilities/DXUtility.h"
#include "View/DXView.h"

#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <gli/dx.hpp>

namespace {

const GUID kRenderdocUuid = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
const GUID kPixUuid = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };
const GUID kGpaUuid = { 0xccffef16, 0x7b69, 0x468f, { 0xbc, 0xe3, 0xcd, 0x95, 0x33, 0x69, 0xa3, 0x9a } };

} // namespace

D3D12_RESOURCE_STATES ConvertState(ResourceState state)
{
    static std::pair<ResourceState, D3D12_RESOURCE_STATES> mapping[] = {
        { ResourceState::kCommon, D3D12_RESOURCE_STATE_COMMON },
        { ResourceState::kVertexAndConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER },
        { ResourceState::kIndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER },
        { ResourceState::kRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET },
        { ResourceState::kUnorderedAccess, D3D12_RESOURCE_STATE_UNORDERED_ACCESS },
        { ResourceState::kDepthStencilWrite, D3D12_RESOURCE_STATE_DEPTH_WRITE },
        { ResourceState::kDepthStencilRead, D3D12_RESOURCE_STATE_DEPTH_READ },
        { ResourceState::kNonPixelShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE },
        { ResourceState::kPixelShaderResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
        { ResourceState::kIndirectArgument, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT },
        { ResourceState::kCopyDest, D3D12_RESOURCE_STATE_COPY_DEST },
        { ResourceState::kCopySource, D3D12_RESOURCE_STATE_COPY_SOURCE },
        { ResourceState::kRaytracingAccelerationStructure, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE },
        { ResourceState::kShadingRateSource, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE },
        { ResourceState::kPresent, D3D12_RESOURCE_STATE_PRESENT },
        { ResourceState::kGenericRead, D3D12_RESOURCE_STATE_GENERIC_READ },
    };

    D3D12_RESOURCE_STATES res = {};
    for (const auto& m : mapping) {
        if (state & m.first) {
            res |= m.second;
            state &= ~m.first;
        }
    }
    assert(state == 0);
    return res;
}

D3D12_HEAP_TYPE GetHeapType(MemoryType memory_type)
{
    switch (memory_type) {
    case MemoryType::kDefault:
        return D3D12_HEAP_TYPE_DEFAULT;
    case MemoryType::kUpload:
        return D3D12_HEAP_TYPE_UPLOAD;
    case MemoryType::kReadback:
        return D3D12_HEAP_TYPE_READBACK;
    default:
        assert(false);
        return D3D12_HEAP_TYPE_CUSTOM;
    }
}

D3D12_RAYTRACING_GEOMETRY_DESC FillRaytracingGeometryDesc(const BufferDesc& vertex,
                                                          const BufferDesc& index,
                                                          RaytracingGeometryFlags flags)
{
    D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = {};

    auto vertex_res = std::static_pointer_cast<DXResource>(vertex.res);
    auto index_res = std::static_pointer_cast<DXResource>(index.res);

    geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    switch (flags) {
    case RaytracingGeometryFlags::kOpaque:
        geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        break;
    case RaytracingGeometryFlags::kNoDuplicateAnyHitInvocation:
        geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
        break;
    }

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;
    geometry_desc.Triangles.VertexBuffer.StartAddress =
        vertex_res->resource->GetGPUVirtualAddress() + vertex.offset * vertex_stride;
    geometry_desc.Triangles.VertexBuffer.StrideInBytes = vertex_stride;
    geometry_desc.Triangles.VertexFormat = static_cast<DXGI_FORMAT>(gli::dx().translate(vertex.format).DXGIFormat.DDS);
    geometry_desc.Triangles.VertexCount = vertex.count;
    if (index_res) {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry_desc.Triangles.IndexBuffer = index_res->resource->GetGPUVirtualAddress() + index.offset * index_stride;
        geometry_desc.Triangles.IndexFormat =
            static_cast<DXGI_FORMAT>(gli::dx().translate(index.format).DXGIFormat.DDS);
        geometry_desc.Triangles.IndexCount = index.count;
    }

    return geometry_desc;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Convert(BuildAccelerationStructureFlags flags)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS dx_flags = {};
    if (flags & BuildAccelerationStructureFlags::kAllowUpdate) {
        dx_flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }
    if (flags & BuildAccelerationStructureFlags::kAllowCompaction) {
        dx_flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    }
    if (flags & BuildAccelerationStructureFlags::kPreferFastTrace) {
        dx_flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    }
    if (flags & BuildAccelerationStructureFlags::kPreferFastBuild) {
        dx_flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    }
    if (flags & BuildAccelerationStructureFlags::kMinimizeMemory) {
        dx_flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
    }
    return dx_flags;
}

DXDevice::DXDevice(DXAdapter& adapter)
    : m_adapter(adapter)
    , m_cpu_descriptor_pool(*this)
    , m_gpu_descriptor_pool(*this)
{
    ASSERT_SUCCEEDED(D3D12CreateDevice(m_adapter.GetAdapter().Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));
    m_device.As(&m_device5);

    ComPtr<IUnknown> renderdoc;
    if (SUCCEEDED(m_device->QueryInterface(kRenderdocUuid, &renderdoc))) {
        m_is_under_graphics_debugger |= !!renderdoc;
    }

    ComPtr<IUnknown> pix;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, kPixUuid, &pix))) {
        m_is_under_graphics_debugger |= !!pix;
    }

    ComPtr<IUnknown> gpa;
    if (SUCCEEDED(m_device->QueryInterface(kGpaUuid, &gpa))) {
        m_is_under_graphics_debugger |= !!gpa;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_support5 = {};
    if (SUCCEEDED(
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_support5, sizeof(feature_support5)))) {
        m_is_dxr_supported = feature_support5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
        m_is_render_passes_supported = feature_support5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
        m_is_ray_query_supported = feature_support5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 feature_support6 = {};
    if (SUCCEEDED(
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &feature_support6, sizeof(feature_support6)))) {
        m_is_variable_rate_shading_supported =
            feature_support6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
        m_shading_rate_image_tile_size = feature_support6.ShadingRateImageTileSize;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 feature_support7 = {};
    if (SUCCEEDED(
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &feature_support7, sizeof(feature_support7)))) {
        m_is_create_not_zeroed_available = true;
        m_is_mesh_shading_supported = feature_support7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
    }

    m_command_queues[CommandListType::kGraphics] = std::make_shared<DXCommandQueue>(*this, CommandListType::kGraphics);
    m_command_queues[CommandListType::kCompute] = std::make_shared<DXCommandQueue>(*this, CommandListType::kCompute);
    m_command_queues[CommandListType::kCopy] = std::make_shared<DXCommandQueue>(*this, CommandListType::kCopy);

    static const bool debug_enabled = IsDebuggerPresent();
    if (debug_enabled) {
        ComPtr<ID3D12InfoQueue> info_queue;
        if (SUCCEEDED(m_device.As(&info_queue))) {
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

            D3D12_MESSAGE_SEVERITY severities[] = {
                D3D12_MESSAGE_SEVERITY_INFO,
            };

            D3D12_MESSAGE_ID deny_ids[] = {
                D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumSeverities = std::size(severities);
            filter.DenyList.pSeverityList = severities;
            filter.DenyList.NumIDs = std::size(deny_ids);
            filter.DenyList.pIDList = deny_ids;
            info_queue->PushStorageFilter(&filter);
        }

        /*ComPtr<ID3D12DebugDevice2> debug_device;
        m_device.As(&debug_device);
        D3D12_DEBUG_FEATURE debug_feature = D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING;
        debug_device->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &debug_feature,
        sizeof(debug_feature));*/
    }
}

std::shared_ptr<Memory> DXDevice::AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
{
    return std::make_shared<DXMemory>(*this, size, memory_type, memory_type_bits);
}

std::shared_ptr<CommandQueue> DXDevice::GetCommandQueue(CommandListType type)
{
    return m_command_queues.at(type);
}

uint32_t DXDevice::GetTextureDataPitchAlignment() const
{
    return D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
}

std::shared_ptr<Swapchain> DXDevice::CreateSwapchain(WindowHandle window,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     uint32_t frame_count,
                                                     bool vsync)
{
    return std::make_shared<DXSwapchain>(*m_command_queues.at(CommandListType::kGraphics), window, width, height,
                                         frame_count, vsync);
}

std::shared_ptr<CommandList> DXDevice::CreateCommandList(CommandListType type)
{
    return std::make_shared<DXCommandList>(*this, type);
}

std::shared_ptr<Fence> DXDevice::CreateFence(uint64_t initial_value)
{
    return std::make_shared<DXFence>(*this, initial_value);
}

std::shared_ptr<Resource> DXDevice::CreateTexture(TextureType type,
                                                  uint32_t bind_flag,
                                                  gli::format format,
                                                  uint32_t sample_count,
                                                  int width,
                                                  int height,
                                                  int depth,
                                                  int mip_levels)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    if (bind_flag & BindFlag::kShaderResource) {
        dx_format = MakeTypelessDepthStencil(dx_format);
    }

    std::shared_ptr<DXResource> res = std::make_shared<DXResource>(*this);
    res->resource_type = ResourceType::kTexture;
    res->format = format;

    D3D12_RESOURCE_DESC desc = {};
    switch (type) {
    case TextureType::k1D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case TextureType::k2D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case TextureType::k3D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    }
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = depth;
    desc.MipLevels = mip_levels;
    desc.Format = dx_format;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_check_desc = {};
    ms_check_desc.Format = desc.Format;
    ms_check_desc.SampleCount = sample_count;
    m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_check_desc, sizeof(ms_check_desc));
    desc.SampleDesc.Count = sample_count;
    desc.SampleDesc.Quality = ms_check_desc.NumQualityLevels - 1;

    if (bind_flag & BindFlag::kRenderTarget) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (bind_flag & BindFlag::kDepthStencil) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (bind_flag & BindFlag::kUnorderedAccess) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    res->desc = desc;
    res->SetInitialState(ResourceState::kCommon);
    return res;
}

std::shared_ptr<Resource> DXDevice::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size)
{
    if (buffer_size == 0) {
        return {};
    }

    std::shared_ptr<DXResource> res = std::make_shared<DXResource>(*this);

    if (bind_flag & BindFlag::kConstantBuffer) {
        buffer_size = (buffer_size + 255) & ~255;
    }

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

    res->resource_type = ResourceType::kBuffer;

    ResourceState state = ResourceState::kCommon;
    if (bind_flag & BindFlag::kRenderTarget) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (bind_flag & BindFlag::kDepthStencil) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (bind_flag & BindFlag::kUnorderedAccess) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (bind_flag & BindFlag::kAccelerationStructure) {
        state = ResourceState::kRaytracingAccelerationStructure;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    res->desc = desc;
    res->SetInitialState(state);
    return res;
}

std::shared_ptr<Resource> DXDevice::CreateSampler(const SamplerDesc& desc)
{
    std::shared_ptr<DXResource> res = std::make_shared<DXResource>(*this);
    D3D12_SAMPLER_DESC& sampler_desc = res->sampler_desc;

    switch (desc.filter) {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }

    switch (desc.mode) {
    case SamplerTextureAddressMode::kWrap:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    }

    switch (desc.func) {
    case SamplerComparisonFunc::kNever:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    }

    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = std::numeric_limits<float>::max();
    sampler_desc.MaxAnisotropy = 1;

    return res;
}

std::shared_ptr<View> DXDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_shared<DXView>(*this, std::static_pointer_cast<DXResource>(resource), view_desc);
}

std::shared_ptr<BindingSetLayout> DXDevice::CreateBindingSetLayout(const std::vector<BindKey>& descs)
{
    return std::make_shared<DXBindingSetLayout>(*this, descs);
}

std::shared_ptr<BindingSet> DXDevice::CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout)
{
    return std::make_shared<DXBindingSet>(*this, std::static_pointer_cast<DXBindingSetLayout>(layout));
}

std::shared_ptr<RenderPass> DXDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    return std::make_shared<DXRenderPass>(desc);
}

std::shared_ptr<Framebuffer> DXDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    return std::make_shared<DXFramebuffer>(desc);
}

std::shared_ptr<Shader> DXDevice::CreateShader(const std::vector<uint8_t>& blob,
                                               ShaderBlobType blob_type,
                                               ShaderType shader_type)
{
    return std::make_shared<ShaderBase>(blob, blob_type, shader_type);
}

std::shared_ptr<Shader> DXDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<ShaderBase>(desc, ShaderBlobType::kDXIL);
}

std::shared_ptr<Program> DXDevice::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return std::make_shared<ProgramBase>(shaders);
}

std::shared_ptr<Pipeline> DXDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    return std::make_shared<DXGraphicsPipeline>(*this, desc);
}

std::shared_ptr<Pipeline> DXDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    return std::make_shared<DXComputePipeline>(*this, desc);
}

std::shared_ptr<Pipeline> DXDevice::CreateRayTracingPipeline(const RayTracingPipelineDesc& desc)
{
    return std::make_shared<DXRayTracingPipeline>(*this, desc);
}

std::shared_ptr<Resource> DXDevice::CreateAccelerationStructure(AccelerationStructureType type,
                                                                const std::shared_ptr<Resource>& resource,
                                                                uint64_t offset)
{
    std::shared_ptr<DXResource> res = std::make_shared<DXResource>(*this);
    res->resource_type = ResourceType::kAccelerationStructure;
    res->acceleration_structures_memory = resource;
    res->acceleration_structure_handle = resource->As<DXResource>().resource->GetGPUVirtualAddress() + offset;
    return res;
}

std::shared_ptr<QueryHeap> DXDevice::CreateQueryHeap(QueryHeapType type, uint32_t count)
{
    if (type == QueryHeapType::kAccelerationStructureCompactedSize) {
        return std::make_shared<DXRayTracingQueryHeap>(*this, type, count);
    }
    return {};
}

RaytracingASPrebuildInfo DXDevice::GetAccelerationStructurePrebuildInfo(
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    m_device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
    RaytracingASPrebuildInfo prebuild_info = {};
    prebuild_info.acceleration_structure_size = info.ResultDataMaxSizeInBytes;
    prebuild_info.build_scratch_data_size = info.ScratchDataSizeInBytes;
    prebuild_info.update_scratch_data_size = info.UpdateScratchDataSizeInBytes;
    return prebuild_info;
}

bool DXDevice::IsDxrSupported() const
{
    return m_is_dxr_supported;
}

bool DXDevice::IsRayQuerySupported() const
{
    return m_is_ray_query_supported;
}

bool DXDevice::IsVariableRateShadingSupported() const
{
    return m_is_variable_rate_shading_supported;
}

bool DXDevice::IsMeshShadingSupported() const
{
    return m_is_mesh_shading_supported;
}

bool DXDevice::IsDrawIndirectCountSupported() const
{
    return true;
}

bool DXDevice::IsGeometryShaderSupported() const
{
    return true;
}

uint32_t DXDevice::GetShadingRateImageTileSize() const
{
    return m_shading_rate_image_tile_size;
}

MemoryBudget DXDevice::GetMemoryBudget() const
{
    ComPtr<IDXGIAdapter3> adapter3;
    m_adapter.GetAdapter().As(&adapter3);
    DXGI_QUERY_VIDEO_MEMORY_INFO local_memory_info = {};
    adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local_memory_info);
    DXGI_QUERY_VIDEO_MEMORY_INFO non_local_memory_info = {};
    adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &non_local_memory_info);
    return { local_memory_info.Budget + non_local_memory_info.Budget,
             local_memory_info.CurrentUsage + non_local_memory_info.CurrentUsage };
}

uint32_t DXDevice::GetShaderGroupHandleSize() const
{
    return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
}

uint32_t DXDevice::GetShaderRecordAlignment() const
{
    return D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
}

uint32_t DXDevice::GetShaderTableAlignment() const
{
    return D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
}

RaytracingASPrebuildInfo DXDevice::GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs,
                                                       BuildAccelerationStructureFlags flags) const
{
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descs;
    for (const auto& desc : descs) {
        geometry_descs.emplace_back(FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags));
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = geometry_descs.size();
    inputs.pGeometryDescs = geometry_descs.data();
    inputs.Flags = Convert(flags);
    return GetAccelerationStructurePrebuildInfo(inputs);
}

RaytracingASPrebuildInfo DXDevice::GetTLASPrebuildInfo(uint32_t instance_count,
                                                       BuildAccelerationStructureFlags flags) const
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = instance_count;
    inputs.Flags = Convert(flags);
    return GetAccelerationStructurePrebuildInfo(inputs);
}

ShaderBlobType DXDevice::GetSupportedShaderBlobType() const
{
    return ShaderBlobType::kDXIL;
}

DXAdapter& DXDevice::GetAdapter()
{
    return m_adapter;
}

ComPtr<ID3D12Device> DXDevice::GetDevice()
{
    return m_device;
}

DXCPUDescriptorPool& DXDevice::GetCPUDescriptorPool()
{
    return m_cpu_descriptor_pool;
}

DXGPUDescriptorPool& DXDevice::GetGPUDescriptorPool()
{
    return m_gpu_descriptor_pool;
}

bool DXDevice::IsRenderPassesSupported() const
{
    return m_is_render_passes_supported;
}

bool DXDevice::IsUnderGraphicsDebugger() const
{
    return m_is_under_graphics_debugger;
}

bool DXDevice::IsCreateNotZeroedAvailable() const
{
    return m_is_create_not_zeroed_available;
}

ID3D12CommandSignature* DXDevice::GetCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride)
{
    auto it = m_command_signature_cache.find({ type, stride });
    if (it != m_command_signature_cache.end()) {
        return it->second.Get();
    }

    D3D12_INDIRECT_ARGUMENT_DESC arg = {};
    arg.Type = type;
    D3D12_COMMAND_SIGNATURE_DESC desc = {};
    desc.NumArgumentDescs = 1;
    desc.pArgumentDescs = &arg;
    desc.ByteStride = stride;
    ComPtr<ID3D12CommandSignature> command_signature;
    ASSERT_SUCCEEDED(m_device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&command_signature)));

    m_command_signature_cache.emplace(std::piecewise_construct, std::forward_as_tuple(type, stride),
                                      std::forward_as_tuple(command_signature));
    return command_signature.Get();
}
