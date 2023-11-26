#include "Device/SWDevice.h"
#include "Adapter/SWAdapter.h"
#include "Swapchain/SWSwapchain.h"
#include "CommandList/SWCommandList.h"
#include "Fence/SWFence.h"
#include "View/SWView.h"
#include "Resource/SWResource.h"
#include "BindingSetLayout/SWBindingSetLayout.h"
#include "BindingSet/SWBindingSet.h"
#include "RenderPass/DXRenderPass.h"
#include "Framebuffer/DXFramebuffer.h"
#include "Program/ProgramBase.h"
#include "Pipeline/SWGraphicsPipeline.h"
#include "Shader/ShaderBase.h"
#include "CommandQueue/SWCommandQueue.h"

SWDevice::SWDevice(SWAdapter& adapter)
    : m_adapter(adapter)
{
}

std::shared_ptr<Memory> SWDevice::AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
{
    return {};
}

std::shared_ptr<CommandQueue> SWDevice::GetCommandQueue(CommandListType type)
{
    return std::make_shared<SWCommandQueue>(*this, type);
}

uint32_t SWDevice::GetTextureDataPitchAlignment() const
{
    return 0;
}

std::shared_ptr<Swapchain> SWDevice::CreateSwapchain(WindowHandle window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync)
{
    return std::make_shared<SWSwapchain>(*this, window, width, height, frame_count, vsync);
}

std::shared_ptr<CommandList> SWDevice::CreateCommandList(CommandListType type)
{
    return std::make_shared<SWCommandList>(*this, type);
}

std::shared_ptr<Fence> SWDevice::CreateFence(uint64_t initial_value)
{
    return std::make_shared<SWFence>(*this, initial_value);
}

std::shared_ptr<Resource> SWDevice::CreateTexture(TextureType type, uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels)
{
    return {};
}

std::shared_ptr<Resource> SWDevice::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size)
{
    return {};
}

std::shared_ptr<Resource> SWDevice::CreateSampler(const SamplerDesc& desc)
{
    return {};
}

std::shared_ptr<View> SWDevice::CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
{
    return std::make_shared<SWView>(*this, std::static_pointer_cast<SWResource>(resource), view_desc);
}

std::shared_ptr<BindingSetLayout> SWDevice::CreateBindingSetLayout(const std::vector<BindKey>& descs)
{
    return std::make_shared<SWBindingSetLayout>(*this, descs);
}

std::shared_ptr<BindingSet> SWDevice::CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout)
{
    return std::make_shared<SWBindingSet>(*this, std::static_pointer_cast<SWBindingSetLayout>(layout));
}

std::shared_ptr<RenderPass> SWDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    return std::make_shared<DXRenderPass>(desc);
}

std::shared_ptr<Framebuffer> SWDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    return std::make_shared<DXFramebuffer>(desc);
}

std::shared_ptr<Shader> SWDevice::CreateShader(const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type)
{
    return std::make_shared<ShaderBase>(blob, blob_type, shader_type);
}

std::shared_ptr<Shader> SWDevice::CompileShader(const ShaderDesc& desc)
{
    return std::make_shared<ShaderBase>(desc, ShaderBlobType::kDXIL);
}

std::shared_ptr<Program> SWDevice::CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders)
{
    return std::make_shared<ProgramBase>(shaders);
}

std::shared_ptr<Pipeline> SWDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    return std::make_shared<SWGraphicsPipeline>(*this, desc);
}

std::shared_ptr<Pipeline> SWDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    return {};
}

std::shared_ptr<Pipeline> SWDevice::CreateRayTracingPipeline(const RayTracingPipelineDesc& desc)
{
    return {};
}

std::shared_ptr<Resource> SWDevice::CreateAccelerationStructure(AccelerationStructureType type, const std::shared_ptr<Resource>& resource, uint64_t offset)
{
    return {};
}

std::shared_ptr<QueryHeap> SWDevice::CreateQueryHeap(QueryHeapType type, uint32_t count)
{
    return {};
}

bool SWDevice::IsDxrSupported() const
{
    return false;
}

bool SWDevice::IsRayQuerySupported() const
{
    return false;
}

bool SWDevice::IsVariableRateShadingSupported() const
{
    return false;
}

bool SWDevice::IsMeshShadingSupported() const
{
    return false;
}

bool SWDevice::IsDrawIndirectCountSupported() const
{
    return false;
}

bool SWDevice::IsGeometryShaderSupported() const
{
    return false;
}

uint32_t SWDevice::GetShadingRateImageTileSize() const
{
    return 0;
}

MemoryBudget SWDevice::GetMemoryBudget() const
{
    return {};
}

uint32_t SWDevice::GetShaderGroupHandleSize() const
{
    return 0;
}

uint32_t SWDevice::GetShaderRecordAlignment() const
{
    return 0;
}

uint32_t SWDevice::GetShaderTableAlignment() const
{
    return 0;
}

RaytracingASPrebuildInfo SWDevice::GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags) const
{
    return {};
}

RaytracingASPrebuildInfo SWDevice::GetTLASPrebuildInfo(uint32_t instance_count, BuildAccelerationStructureFlags flags) const
{
    return {};
}
