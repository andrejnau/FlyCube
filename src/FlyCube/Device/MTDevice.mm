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
#include "QueryHeap/MTQueryHeap.h"
#include "Resource/MTAccelerationStructure.h"
#include "Resource/MTBuffer.h"
#include "Resource/MTSampler.h"
#include "Resource/MTTexture.h"
#include "Shader/MTShader.h"
#include "Swapchain/MTSwapchain.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"
#include "View/MTView.h"

#include <type_traits>

MTLCompareFunction ConvertToCompareFunction(ComparisonFunc func)
{
    switch (func) {
    case ComparisonFunc::kNever:
        return MTLCompareFunctionNever;
    case ComparisonFunc::kLess:
        return MTLCompareFunctionLess;
    case ComparisonFunc::kEqual:
        return MTLCompareFunctionEqual;
    case ComparisonFunc::kLessEqual:
        return MTLCompareFunctionLessEqual;
    case ComparisonFunc::kGreater:
        return MTLCompareFunctionGreater;
    case ComparisonFunc::kNotEqual:
        return MTLCompareFunctionNotEqual;
    case ComparisonFunc::kGreaterEqual:
        return MTLCompareFunctionGreaterEqual;
    case ComparisonFunc::kAlways:
        return MTLCompareFunctionAlways;
    default:
        NOTREACHED();
    }
}

MTDevice::MTDevice(MTInstance& instance, id<MTLDevice> device)
    : instance_(instance)
    , device_(device)
    , mvk_pixel_formats_(this)
    , bindless_argument_buffer_(*this)
{
    command_queue_ = std::make_shared<MTCommandQueue>(*this);

    NSError* error = nullptr;
    compiler_ = [device_ newCompilerWithDescriptor:[MTL4CompilerDescriptor new] error:&error];
    if (!compiler_) {
        Logging::Println("Failed to create MTL4Compiler: {}", error);
    }
}

std::shared_ptr<Memory> MTDevice::AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
{
    return std::make_shared<MTMemory>(*this, size, memory_type);
}

std::shared_ptr<CommandQueue> MTDevice::GetCommandQueue(CommandListType type)
{
    return command_queue_;
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
    return MTTexture::CreateTexture(*this, desc)->GetMemoryRequirements();
}

MemoryRequirements MTDevice::GetMemoryBufferRequirements(const BufferDesc& desc)
{
    return MTBuffer::CreateBuffer(*this, desc)->GetMemoryRequirements();
}

std::shared_ptr<Resource> MTDevice::CreatePlacedTexture(const std::shared_ptr<Memory>& memory,
                                                        uint64_t offset,
                                                        const TextureDesc& desc)
{
    auto texture = MTTexture::CreateTexture(*this, desc);
    if (texture) {
        texture->BindMemory(memory, offset);
    }
    return texture;
}

std::shared_ptr<Resource> MTDevice::CreatePlacedBuffer(const std::shared_ptr<Memory>& memory,
                                                       uint64_t offset,
                                                       const BufferDesc& desc)
{
    auto buffer = MTBuffer::CreateBuffer(*this, desc);
    if (buffer) {
        buffer->BindMemory(memory, offset);
    }
    return buffer;
}

std::shared_ptr<Resource> MTDevice::CreateTexture(MemoryType memory_type, const TextureDesc& desc)
{
    auto texture = MTTexture::CreateTexture(*this, desc);
    if (texture) {
        texture->CommitMemory(memory_type);
    }
    return texture;
}

std::shared_ptr<Resource> MTDevice::CreateBuffer(MemoryType memory_type, const BufferDesc& desc)
{
    auto buffer = MTBuffer::CreateBuffer(*this, desc);
    if (buffer) {
        buffer->CommitMemory(memory_type);
    }
    return buffer;
}

std::shared_ptr<Resource> MTDevice::CreateSampler(const SamplerDesc& desc)
{
    return MTSampler::CreateSampler(*this, desc);
}

std::shared_ptr<View> MTDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_shared<MTView>(*this, std::static_pointer_cast<MTResource>(resource), view_desc);
}

std::shared_ptr<BindingSetLayout> MTDevice::CreateBindingSetLayout(const BindingSetLayoutDesc& desc)
{
    return std::make_shared<MTBindingSetLayout>(*this, desc);
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
    return MTAccelerationStructure::CreateAccelerationStructure(*this, desc);
}

std::shared_ptr<QueryHeap> MTDevice::CreateQueryHeap(QueryHeapType type, uint32_t count)
{
    if (type == QueryHeapType::kAccelerationStructureCompactedSize) {
        return std::make_shared<MTQueryHeap>(*this, type, count);
    }
    return nullptr;
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

bool MTDevice::IsSamplerFilterMinmaxSupported() const
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

MTL4AccelerationStructureTriangleGeometryDescriptor* FillRaytracingGeometryDesc(
    const RaytracingGeometryBufferDesc& vertex,
    const RaytracingGeometryBufferDesc& index,
    RaytracingGeometryFlags flags)
{
    MTL4AccelerationStructureTriangleGeometryDescriptor* geometry_desc =
        [MTL4AccelerationStructureTriangleGeometryDescriptor new];

    geometry_desc.opaque =
        static_cast<std::underlying_type_t<RaytracingGeometryFlags>>(flags) &
        static_cast<std::underlying_type_t<RaytracingGeometryFlags>>(RaytracingGeometryFlags::kOpaque);

    auto vertex_res = std::static_pointer_cast<MTResource>(vertex.res);
    auto index_res = std::static_pointer_cast<MTResource>(index.res);

    auto vertex_stride = gli::detail::bits_per_pixel(vertex.format) / 8;
    geometry_desc.vertexBuffer = vertex_res->GetBuffer().gpuAddress + vertex.offset * vertex_stride;
    geometry_desc.vertexStride = vertex_stride;
    geometry_desc.triangleCount = vertex.count / 3;

    if (index_res) {
        auto index_stride = gli::detail::bits_per_pixel(index.format) / 8;
        geometry_desc.indexBuffer = index_res->GetBuffer().gpuAddress + index.offset * index_stride;
        geometry_desc.triangleCount = index.count / 3;
    }

    return geometry_desc;
}

RaytracingASPrebuildInfo MTDevice::GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs,
                                                       BuildAccelerationStructureFlags flags) const
{
    NSMutableArray* geometry_descs = [NSMutableArray array];
    for (const auto& desc : descs) {
        MTL4AccelerationStructureTriangleGeometryDescriptor* geometry_desc =
            FillRaytracingGeometryDesc(desc.vertex, desc.index, desc.flags);
        [geometry_descs addObject:geometry_desc];
    }

    MTL4PrimitiveAccelerationStructureDescriptor* acceleration_structure_desc =
        [MTL4PrimitiveAccelerationStructureDescriptor new];
    acceleration_structure_desc.geometryDescriptors = geometry_descs;

    MTLAccelerationStructureSizes sizes =
        [device_ accelerationStructureSizesWithDescriptor:acceleration_structure_desc];

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
        [device_ accelerationStructureSizesWithDescriptor:acceleration_structure_desc];

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

uint64_t MTDevice::GetConstantBufferOffsetAlignment() const
{
    return 16;
}

id<MTLDevice> MTDevice::GetDevice() const
{
    return device_;
}

MTLPixelFormat MTDevice::GetMTLPixelFormat(gli::format format)
{
    return mvk_pixel_formats_.getMTLPixelFormat(static_cast<VkFormat>(format));
}

MTLVertexFormat MTDevice::GetMTLVertexFormat(gli::format format)
{
    return mvk_pixel_formats_.getMTLVertexFormat(static_cast<VkFormat>(format));
}

id<MTL4CommandQueue> MTDevice::GetMTCommandQueue() const
{
    return command_queue_->GetCommandQueue();
}

uint32_t MTDevice::GetMaxPerStageBufferCount() const
{
    return 31;
}

const MTInstance& MTDevice::GetInstance()
{
    return instance_;
}

MTGPUBindlessArgumentBuffer& MTDevice::GetBindlessArgumentBuffer()
{
    return bindless_argument_buffer_;
}

id<MTLResidencySet> MTDevice::CreateResidencySet() const
{
    NSError* error = nullptr;
    MTLResidencySetDescriptor* residencySetDescriptor = [MTLResidencySetDescriptor new];
    return [device_ newResidencySetWithDescriptor:residencySetDescriptor error:&error];
}

id<MTL4Compiler> MTDevice::GetCompiler()
{
    return compiler_;
}

MVKVulkanAPIObject* MTDevice::getVulkanAPIObject()
{
    return this;
}

id<MTLDevice> MTDevice::getMTLDevice()
{
    return device_;
}

const MVKPhysicalDeviceMetalFeatures* MTDevice::getMetalFeatures()
{
    return &features_;
}
