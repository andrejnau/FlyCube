#include "Device/MTDevice.h"
#include <CommandQueue/MTCommandQueue.h>
#include <Swapchain/MTSwapchain.h>
#include <Shader/ShaderBase.h>
#include <Program/ProgramBase.h>
#include <Pipeline/MTComputePipeline.h>
#include <Pipeline/MTGraphicsPipeline.h>
#include <CommandList/MTCommandList.h>
#include <Framebuffer/MTFramebuffer.h>
#include <RenderPass/MTRenderPass.h>
#include <View/MTView.h>
#include <Resource/MTResource.h>
#include <Fence/MTFence.h>
#include <BindingSetLayout/MTBindingSetLayout.h>
#include <BindingSet/MTBindingSet.h>
#include <Memory/MTMemory.h>

MTDevice::MTDevice(const id<MTLDevice>& device)
    : m_device(device)
    , m_mvk_pixel_formats(this)
{
    m_command_queue = std::make_shared<MTCommandQueue>(m_device);
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

std::shared_ptr<Swapchain> MTDevice::CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync)
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

std::shared_ptr<Resource> MTDevice::CreateTexture(TextureType type, uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels)
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
    if (buffer_size == 0)
        return {};

    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(*this);
    res->resource_type = ResourceType::kBuffer;
    res->buffer.size = buffer_size;
    return res;
}

std::shared_ptr<Resource> MTDevice::CreateSampler(const SamplerDesc& desc)
{
    std::shared_ptr<MTResource> res = std::make_shared<MTResource>(*this);
    res->resource_type = ResourceType::kSampler;
    
    // TODO: Fix sampler param mapping
    MTLSamplerDescriptor* sampler_descriptor = [MTLSamplerDescriptor new];
    sampler_descriptor.minFilter = MTLSamplerMinMagFilterLinear;
    sampler_descriptor.magFilter = MTLSamplerMinMagFilterLinear;
    sampler_descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
    sampler_descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
    sampler_descriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;
    
    switch (desc.func)
    {
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

std::shared_ptr<Shader> MTDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<ShaderBase>(desc, ShaderBlobType::kSPIRV);
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

std::shared_ptr<Resource> MTDevice::CreateAccelerationStructure(AccelerationStructureType type, const std::shared_ptr<Resource>& resource, uint64_t offset)
{
    assert(false);
    return {};
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
    return false;
}

bool MTDevice::IsVariableRateShadingSupported() const
{
    return false;
}

bool MTDevice::IsMeshShadingSupported() const
{
    return false;
}

uint32_t MTDevice::GetShadingRateImageTileSize() const
{
    return false;
}

MemoryBudget MTDevice::GetMemoryBudget() const
{
    return {};
}

uint32_t MTDevice::GetShaderGroupHandleSize() const
{
    return 0;
}

uint32_t MTDevice::GetShaderRecordAlignment() const
{
    return 0;
}

uint32_t MTDevice::GetShaderTableAlignment() const
{
    return 0;
}

RaytracingASPrebuildInfo MTDevice::GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags) const
{
    return {};
}

RaytracingASPrebuildInfo MTDevice::GetTLASPrebuildInfo(uint32_t instance_count, BuildAccelerationStructureFlags flags) const
{
    return {};
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
