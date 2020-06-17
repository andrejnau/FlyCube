#pragma once
#include <Instance/QueryInterface.h>
#include <Swapchain/Swapchain.h>
#include <CommandList/CommandList.h>
#include <Fence/Fence.h>
#include <Semaphore/Semaphore.h>
#include <Instance/BaseTypes.h>
#include <Program/Program.h>
#include <Framebuffer/Framebuffer.h>
#include <Pipeline/Pipeline.h>
#include <Shader/Shader.h>
#include <memory>
#include <vector>
#include <GLFW/glfw3.h>
#include <gli/format.hpp>

class Device : public QueryInterface
{
public:
    virtual ~Device() = default;
    virtual uint32_t GetTextureDataPitchAlignment() const = 0;
    virtual std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync) = 0;
    virtual std::shared_ptr<CommandList> CreateCommandList() = 0;
    virtual std::shared_ptr<Fence> CreateFence(uint64_t initial_value) = 0;
    virtual std::shared_ptr<Semaphore> CreateGPUSemaphore() = 0;
    virtual std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;
    virtual std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, MemoryType memory_type = MemoryType::kDefault) = 0;
    virtual std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) = 0;
    virtual std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) = 0;
    virtual std::shared_ptr<Framebuffer> CreateFramebuffer(const std::shared_ptr<Pipeline>& pipeline, uint32_t width, uint32_t height,
                                                           const std::vector<std::shared_ptr<View>>& rtvs = {}, const std::shared_ptr<View>& dsv = {}) = 0;
    virtual std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) = 0;
    virtual std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) = 0;
    virtual std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) = 0;
    virtual std::shared_ptr<Resource> CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index = {}) = 0;
    virtual std::shared_ptr<Resource> CreateTopLevelAS(uint32_t instance_count) = 0;
    virtual bool IsDxrSupported() const = 0;
    virtual bool IsVariableRateShadingSupported() const = 0;
    virtual uint32_t GetShadingRateImageTileSize() const = 0;
    virtual void Wait(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void Signal(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) = 0;
    virtual void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) = 0;
    virtual void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) = 0;
};
