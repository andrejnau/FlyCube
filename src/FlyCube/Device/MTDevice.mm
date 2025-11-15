#include "Device/MTDevice.h"

#include "BindingSet/MTBindingSet.h"
#include "BindingSetLayout/MTBindingSetLayout.h"
#include "CommandList/MTCommandList.h"
#include "CommandList/RecordCommandList.h"
#include "CommandQueue/MTCommandQueue.h"
#include "Fence/MTFence.h"
#include "HLSLCompiler/Compiler.h"
#include "Instance/MTInstance.h"
#include "Memory/MTMemory.h"
#include "Pipeline/MTComputePipeline.h"
#include "Pipeline/MTGraphicsPipeline.h"
#include "Resource/MTResource.h"
#include "Shader/MTShader.h"
#include "Swapchain/MTSwapchain.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"
#include "View/MTView.h"

MTDevice::MTDevice(MTInstance& instance, id<MTLDevice> device)
    : m_instance(instance)
    , m_device(device)
    , m_mvk_pixel_formats(this)
    , m_bindless_argument_buffer(*this)
{
    m_command_queue = std::make_shared<MTCommandQueue>(*this);

    NSError* error = nullptr;
    m_compiler = [m_device newCompilerWithDescriptor:[MTL4CompilerDescriptor new] error:&error];
    if (!m_compiler) {
        Logging::Println("Failed to create MTL4Compiler: {}", error);
    }
}

std::shared_ptr<Memory> MTDevice::AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
{
    return std::make_shared<MTMemory>(*this, size, memory_type);
}

std::shared_ptr<CommandQueue> MTDevice::GetCommandQueue(CommandListType type)
{
    return m_command_queue;
}

uint32_t MTDevice::GetTextureDataPitchAlignment() const
{
    return 1;
}

std::shared_ptr<Swapchain> MTDevice::CreateSwapchain(const NativeSurface& surface,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     uint32_t frame_count,
                                                     bool vsync)
{
    return std::make_shared<MTSwapchain>(*this, surface, width, height, frame_count, vsync);
}

std::shared_ptr<CommandList> MTDevice::CreateCommandList(CommandListType type)
{
    auto command_list = std::make_unique<MTCommandList>(*this, type);
    return std::make_shared<RecordCommandList<MTCommandList>>(std::move(command_list));
}

std::shared_ptr<Fence> MTDevice::CreateFence(uint64_t initial_value)
{
    return std::make_shared<MTFence>(*this, initial_value);
}

MemoryRequirements MTDevice::GetTextureMemoryRequirements(const TextureDesc& desc)
{
    return MTResource::CreateTexture(*this, desc)->GetMemoryRequirements();
}

MemoryRequirements MTDevice::GetMemoryBufferRequirements(const BufferDesc& desc)
{
    return MTResource::CreateBuffer(*this, desc)->GetMemoryRequirements();
}

std::shared_ptr<Resource> MTDevice::CreatePlacedTexture(const std::shared_ptr<Memory>& memory,
                                                        uint64_t offset,
                                                        const TextureDesc& desc)
{
    auto texture = MTResource::CreateTexture(*this, desc);
    if (texture) {
        texture->BindMemory(memory, offset);
    }
    return texture;
}

std::shared_ptr<Resource> MTDevice::CreatePlacedBuffer(const std::shared_ptr<Memory>& memory,
                                                       uint64_t offset,
                                                       const BufferDesc& desc)
{
    auto buffer = MTResource::CreateBuffer(*this, desc);
    if (buffer) {
        buffer->BindMemory(memory, offset);
    }
    return buffer;
}

std::shared_ptr<Resource> MTDevice::CreateTexture(MemoryType memory_type, const TextureDesc& desc)
{
    auto texture = MTResource::CreateTexture(*this, desc);
    if (texture) {
        texture->CommitMemory(memory_type);
    }
    return texture;
}

std::shared_ptr<Resource> MTDevice::CreateBuffer(MemoryType memory_type, const BufferDesc& desc)
{
    auto buffer = MTResource::CreateBuffer(*this, desc);
    if (buffer) {
        buffer->CommitMemory(memory_type);
    }
    return buffer;
}

std::shared_ptr<Resource> MTDevice::CreateSampler(const SamplerDesc& desc)
{
    return MTResource::CreateSampler(*this, desc);
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

std::shared_ptr<Shader> MTDevice::CreateShader(const std::vector<uint8_t>& blob,
                                               ShaderBlobType blob_type,
                                               ShaderType shader_type)
{
    return std::make_shared<MTShader>(*this, blob, blob_type, shader_type);
}

std::shared_ptr<Shader> MTDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<MTShader>(*this, Compile(desc, ShaderBlobType::kSPIRV), ShaderBlobType::kSPIRV, desc.type);
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
    NOTREACHED();
}

std::shared_ptr<Resource> MTDevice::CreateAccelerationStructure(const AccelerationStructureDesc& desc)
{
    return MTResource::CreateAccelerationStructure(*this, desc);
}

std::shared_ptr<QueryHeap> MTDevice::CreateQueryHeap(QueryHeapType type, uint32_t count)
{
    NOTREACHED();
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
    return true;
}

bool MTDevice::IsDrawIndirectCountSupported() const
{
    return false;
}

bool MTDevice::IsGeometryShaderSupported() const
{
    return false;
}

bool MTDevice::IsBindlessSupported() const
{
    return true;
}

uint32_t MTDevice::GetShadingRateImageTileSize() const
{
    NOTREACHED();
}

MemoryBudget MTDevice::GetMemoryBudget() const
{
    NOTREACHED();
}

uint32_t MTDevice::GetShaderGroupHandleSize() const
{
    NOTREACHED();
}

uint32_t MTDevice::GetShaderRecordAlignment() const
{
    NOTREACHED();
}

uint32_t MTDevice::GetShaderTableAlignment() const
{
    NOTREACHED();
}

MTLAccelerationStructureTriangleGeometryDescriptor* FillRaytracingGeometryDesc(
    const RaytracingGeometryBufferDesc& vertex,
    const RaytracingGeometryBufferDesc& index,
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
    geometry_desc.vertexBuffer = vertex_res->GetBuffer();
    geometry_desc.vertexBufferOffset = vertex.offset * vertex_stride;
    geometry_desc.vertexStride = vertex_stride;
    geometry_desc.triangleCount = vertex.count / 3;

    if (index_res) {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry_desc.indexBuffer = index_res->GetBuffer();
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

id<MTLDevice> MTDevice::GetDevice() const
{
    return m_device;
}

MTLPixelFormat MTDevice::GetMTLPixelFormat(gli::format format)
{
    return m_mvk_pixel_formats.getMTLPixelFormat(static_cast<VkFormat>(format));
}

MTLVertexFormat MTDevice::GetMTLVertexFormat(gli::format format)
{
    return m_mvk_pixel_formats.getMTLVertexFormat(static_cast<VkFormat>(format));
}

id<MTL4CommandQueue> MTDevice::GetMTCommandQueue() const
{
    return m_command_queue->GetCommandQueue();
}

uint32_t MTDevice::GetMaxPerStageBufferCount() const
{
    return 31;
}

const MTInstance& MTDevice::GetInstance()
{
    return m_instance;
}

MTGPUBindlessArgumentBuffer& MTDevice::GetBindlessArgumentBuffer()
{
    return m_bindless_argument_buffer;
}

id<MTLResidencySet> MTDevice::CreateResidencySet() const
{
    NSError* error = nullptr;
    MTLResidencySetDescriptor* residencySetDescriptor = [MTLResidencySetDescriptor new];
    return [m_device newResidencySetWithDescriptor:residencySetDescriptor error:&error];
}

id<MTL4Compiler> MTDevice::GetCompiler()
{
    return m_compiler;
}

MVKVulkanAPIObject* MTDevice::getVulkanAPIObject()
{
    return this;
}

id<MTLDevice> MTDevice::getMTLDevice()
{
    return m_device;
}

const MVKPhysicalDeviceMetalFeatures* MTDevice::getMetalFeatures()
{
    return &m_features;
}
