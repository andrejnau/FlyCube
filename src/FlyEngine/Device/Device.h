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
#include <gli/gli.hpp>

class Device : public QueryInterface
{
public:
    virtual ~Device() = default;
    virtual std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) = 0;
    virtual std::shared_ptr<CommandList> CreateCommandList() = 0;
    virtual std::shared_ptr<Fence> CreateFence() = 0;
    virtual std::shared_ptr<Semaphore> CreateGPUSemaphore() = 0;
    virtual std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) = 0;
    virtual std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) = 0;
    virtual std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) = 0;
    virtual std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) = 0;
    virtual std::shared_ptr<Framebuffer> CreateFramebuffer(const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv = {}) = 0;
    virtual std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) = 0;
    virtual std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) = 0;
    virtual std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual void Wait(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void Signal(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence = {}) = 0;
};
