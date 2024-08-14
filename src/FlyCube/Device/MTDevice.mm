#include "Device/MTDevice.h"

#include "BindingSet/MTBindingSet.h"
#include "BindingSetLayout/MTBindingSetLayout.h"
#include "CommandList/MTCommandList.h"
#include "CommandQueue/MTCommandQueue.h"
#include "Fence/MTFence.h"
#include "Framebuffer/MTFramebuffer.h"
#include "Instance/MTInstance.h"
#include "Memory/MTMemory.h"
#include "Pipeline/MTComputePipeline.h"
#include "Pipeline/MTGraphicsPipeline.h"
#include "Program/ProgramBase.h"
#include "RenderPass/MTRenderPass.h"
#include "Resource/MTResource.h"
#include "Shader/MTShader.h"
#include "Swapchain/MTSwapchain.h"
#include "View/MTView.h"

MTDevice::MTDevice(MTInstance& instance, const id<MTLDevice>& device)
    : m_instance(instance)
    , m_device(device)
    , m_mvk_pixel_formats(this)
    , m_bindless_argument_buffer(*this)
{
    m_command_queue = std::make_shared<MTCommandQueue>(*this);
}

std::shared_ptr<Memory> MTDevice::AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
{
    return std::make_shared<MTMemory>(*this, size, memory_type, memory_type_bits);
}

std::shared_ptr<CommandQueue> MTDevice::GetCommandQueue(CommandListType type)
{
    return m_command_queue;
}

uint32_t MTDevice::GetTextureDataPitchAlignment() const
{
    return 1;
}

std::shared_ptr<Swapchain> MTDevice::CreateSwapchain(WindowHandle window,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     uint32_t frame_count,
                                                     bool vsync)
{
    return std::make_shared<MTSwapchain>(*this, window, width, height, frame_count, vsync);
}

std::shared_ptr<CommandList> MTDevice::CreateCommandList(CommandListType type)
{
    return std::make_shared<MTCommandList>(*this, type);
}

std::shared_ptr<Fence> MTDevice::CreateFence(uint64_t initial_value)
{
    return std::make_shared<MTFence>(*this, initial_value);
}

std::shared_ptr<Resource> MTDevice::CreateTexture(TextureType type,
                                                  uint32_t bind_flag,
                                                  gli::format format,
                                                  uint32_t sample_count,
                                                  int width,
                                                  int height,
                                                  int depth,
                                                  int mip_levels)
{
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(*this);
    res->resource_type = ResourceType::kTexture;
    res->format = format;
    res->texture.type = type;
    res->texture.bind_flag = bind_flag;
    res->texture.format = GetMVKPixelFormats().getMTLPixelFormat(static_cast<VkFormat>(format));
    res->texture.sample_count = sample_count;
    res->texture.width = width;
    res->texture.height = height;
    res->texture.depth = depth;
    res->texture.mip_levels = mip_levels;
    return res;
}

std::shared_ptr<Resource> MTDevice::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size)
{
    if (buffer_size == 0) {
        return {};
    }

    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(*this);
    res->resource_type = ResourceType::kBuffer;
    res->buffer.size = buffer_size;
    return res;
}

std::shared_ptr<Resource> MTDevice::CreateSampler(const SamplerDesc& desc)
{
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(*this);
    res->resource_type = ResourceType::kSampler;

    MTLSamplerDescriptor* sampler_descriptor = [MTLSamplerDescriptor new];
    sampler_descriptor.supportArgumentBuffers = YES;
    sampler_descriptor.minFilter = MTLSamplerMinMagFilterLinear;
    sampler_descriptor.magFilter = MTLSamplerMinMagFilterLinear;
    sampler_descriptor.mipFilter = MTLSamplerMipFilterLinear;
    sampler_descriptor.maxAnisotropy = 16;
    sampler_descriptor.borderColor = MTLSamplerBorderColorOpaqueBlack;
    sampler_descriptor.lodMinClamp = 0;
    sampler_descriptor.lodMaxClamp = std::numeric_limits<float>::max();

    switch (desc.mode) {
    case SamplerTextureAddressMode::kWrap:
        sampler_descriptor.sAddressMode = MTLSamplerAddressModeRepeat;
        sampler_descriptor.tAddressMode = MTLSamplerAddressModeRepeat;
        sampler_descriptor.rAddressMode = MTLSamplerAddressModeRepeat;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler_descriptor.rAddressMode = MTLSamplerAddressModeClampToEdge;
        break;
    }

    switch (desc.func) {
    case SamplerComparisonFunc::kNever:
        sampler_descriptor.compareFunction = MTLCompareFunctionNever;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_descriptor.compareFunction = MTLCompareFunctionAlways;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_descriptor.compareFunction = MTLCompareFunctionLess;
        break;
    }

    res->sampler.res = [m_device newSamplerStateWithDescriptor:sampler_descriptor];
    return res;
}

std::shared_ptr<View> MTDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_shared<MTView>(*this, std::static_pointer_cast<MTResource>(resource), view_desc);
}

std::shared_ptr<BindingSetLayout> MTDevice::CreateBindingSetLayout(const std::vector<BindKey>& descs)
{
    return std::make_shared<MTBindingSetLayout>(*this, descs);
}

std::shared_ptr<BindingSet> MTDevice::CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout)
{
    return std::make_shared<MTBindingSet>(*this, std::static_pointer_cast<MTBindingSetLayout>(layout));
}

std::shared_ptr<RenderPass> MTDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    return std::make_shared<MTRenderPass>(desc);
}

std::shared_ptr<Framebuffer> MTDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    return std::make_shared<MTFramebuffer>(desc);
}

std::shared_ptr<Shader> MTDevice::CreateShader(const std::vector<uint8_t>& blob,
                                               ShaderBlobType blob_type,
                                               ShaderType shader_type)
{
    return std::make_shared<MTShader>(*this, blob, blob_type, shader_type);
}

std::shared_ptr<Shader> MTDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<MTShader>(*this, desc, ShaderBlobType::kSPIRV);
}

std::shared_ptr<Program> MTDevice::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return std::make_shared<ProgramBase>(shaders);
}

std::shared_ptr<Pipeline> MTDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    return std::make_shared<MTGraphicsPipeline>(*this, desc);
}

std::shared_ptr<Pipeline> MTDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    return std::make_shared<MTComputePipeline>(*this, desc);
}

std::shared_ptr<Pipeline> MTDevice::CreateRayTracingPipeline(const RayTracingPipelineDesc& desc)
{
    assert(false);
    return {};
}

std::shared_ptr<Resource> MTDevice::CreateAccelerationStructure(AccelerationStructureType type,
                                                                const std::shared_ptr<Resource>& resource,
                                                                uint64_t offset)
{
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(*this);
    res->resource_type = ResourceType::kAccelerationStructure;
    res->acceleration_structure = [m_device newAccelerationStructureWithSize:resource->GetWidth() - offset];
    res->acceleration_structure_handle = [res->acceleration_structure gpuResourceID];
    return res;
}

std::shared_ptr<QueryHeap> MTDevice::CreateQueryHeap(QueryHeapType type, uint32_t count)
{
    assert(false);
    return {};
}

bool MTDevice::IsDxrSupported() const
{
    return false;
}

bool MTDevice::IsRayQuerySupported() const
{
    return true;
}

bool MTDevice::IsVariableRateShadingSupported() const
{
    return false;
}

bool MTDevice::IsMeshShadingSupported() const
{
    return false;
}

bool MTDevice::IsDrawIndirectCountSupported() const
{
    return false;
}

bool MTDevice::IsGeometryShaderSupported() const
{
    return false;
}

uint32_t MTDevice::GetShadingRateImageTileSize() const
{
    assert(false);
    return false;
}

MemoryBudget MTDevice::GetMemoryBudget() const
{
    assert(false);
    return {};
}

uint32_t MTDevice::GetShaderGroupHandleSize() const
{
    assert(false);
    return 0;
}

uint32_t MTDevice::GetShaderRecordAlignment() const
{
    assert(false);
    return 0;
}

uint32_t MTDevice::GetShaderTableAlignment() const
{
    assert(false);
    return 0;
}

MTLAccelerationStructureTriangleGeometryDescriptor* FillRaytracingGeometryDesc(const BufferDesc& vertex,
                                                                               const BufferDesc& index,
                                                                               RaytracingGeometryFlags flags)
{
    MTLAccelerationStructureTriangleGeometryDescriptor* geometry_desc =
        [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

    auto vertex_res = std::static_pointer_cast<MTResource>(vertex.res);
    auto index_res = std::static_pointer_cast<MTResource>(index.res);

    switch (flags) {
    case RaytracingGeometryFlags::kOpaque:
        geometry_desc.opaque = true;
        break;
    case RaytracingGeometryFlags::kNoDuplicateAnyHitInvocation:
        geometry_desc.allowDuplicateIntersectionFunctionInvocation = false;
        break;
    default:
        break;
    }

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;
    geometry_desc.vertexBuffer = vertex_res->buffer.res;
    geometry_desc.vertexBufferOffset = vertex.offset * vertex_stride;
    geometry_desc.vertexStride = vertex_stride;
    geometry_desc.triangleCount = vertex.count / 3;

    if (index_res) {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry_desc.indexBuffer = index_res->buffer.res;
        geometry_desc.indexBufferOffset = index.offset * index_stride;
        geometry_desc.triangleCount = index.count / 3;
    }

    return geometry_desc;
}

RaytracingASPrebuildInfo MTDevice::GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs,
                                                       BuildAccelerationStructureFlags flags) const
{
    NSMutableArray* geometry_descs = [NSMutableArray array];
    for (const auto& desc : descs) {
        MTLAccelerationStructureTriangleGeometryDescriptor* geometry_desc =
            FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags);
        [geometry_descs addObject:geometry_desc];
    }

    MTLPrimitiveAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTLPrimitiveAccelerationStructureDescriptor descriptor];
    acceleration_structure_desc.geometryDescriptors = geometry_descs;

    MTLAccelerationStructureSizes sizes =
        [m_device accelerationStructureSizesWithDescriptor:acceleration_structure_desc];

    RaytracingASPrebuildInfo prebuild_info = {};
    prebuild_info.acceleration_structure_size = sizes.accelerationStructureSize;
    prebuild_info.build_scratch_data_size = sizes.buildScratchBufferSize;
    prebuild_info.update_scratch_data_size = sizes.refitScratchBufferSize;
    return prebuild_info;
}

RaytracingASPrebuildInfo MTDevice::GetTLASPrebuildInfo(uint32_t instance_count,
                                                       BuildAccelerationStructureFlags flags) const
{
    MTLInstanceAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTLInstanceAccelerationStructureDescriptor descriptor];
    acceleration_structure_desc.instanceCount = instance_count;

    MTLAccelerationStructureSizes sizes =
        [m_device accelerationStructureSizesWithDescriptor:acceleration_structure_desc];

    RaytracingASPrebuildInfo prebuild_info = {};
    prebuild_info.acceleration_structure_size = sizes.accelerationStructureSize;
    prebuild_info.build_scratch_data_size = sizes.buildScratchBufferSize;
    prebuild_info.update_scratch_data_size = sizes.refitScratchBufferSize;
    return prebuild_info;
}

ShaderBlobType MTDevice::GetSupportedShaderBlobType() const
{
    return ShaderBlobType::kSPIRV;
}

const id<MTLDevice>& MTDevice::GetDevice() const
{
    return m_device;
}

id<MTLDevice> MTDevice::getMTLDevice()
{
    return GetDevice();
}

MVKPixelFormats& MTDevice::GetMVKPixelFormats()
{
    return m_mvk_pixel_formats;
}

id<MTLCommandQueue> MTDevice::GetMTCommandQueue() const
{
    return m_command_queue->GetCommandQueue();
}

uint32_t MTDevice::GetMaxPerStageBufferCount() const
{
    return 31;
}

MTInstance& MTDevice::GetInstance()
{
    return m_instance;
}

MTGPUBindlessArgumentBuffer& MTDevice::GetBindlessArgumentBuffer()
{
    return m_bindless_argument_buffer;
}
