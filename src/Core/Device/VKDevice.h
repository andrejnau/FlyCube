#pragma once
#include "Device/Device.h"
#include <Utilities/Vulkan.h>
#include <GPUDescriptorPool/VKGPUDescriptorPool.h>

class VKAdapter;

class VKDevice : public Device
{
public:
    VKDevice(VKAdapter& adapter);
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList() override;
    std::shared_ptr<Fence> CreateFence() override;
    std::shared_ptr<Semaphore> CreateGPUSemaphore() override;
    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels) override;
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<Framebuffer> CreateFramebuffer(const std::shared_ptr<Pipeline>& pipeline, const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) override;
    std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;
    std::shared_ptr<Resource> CreateBottomLevelAS(const std::shared_ptr<CommandList>& command_list, const BufferDesc& vertex, const BufferDesc& index) override;
    std::shared_ptr<Resource> CreateTopLevelAS(const std::shared_ptr<CommandList>& command_list, const std::vector<std::pair<std::shared_ptr<Resource>, glm::mat4>>& geometry) override;
    bool IsDxrSupported() const override;
    void Wait(const std::shared_ptr<Semaphore>& semaphore) override;
    void Signal(const std::shared_ptr<Semaphore>& semaphore) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence) override;

    VKAdapter& GetAdapter();
    uint32_t GetQueueFamilyIndex();
    vk::Device GetDevice();
    vk::Queue GetQueue();
    vk::CommandPool GetCmdPool();
    vk::ImageAspectFlags GetAspectFlags(vk::Format format) const;
    VKGPUDescriptorPool& GetGPUDescriptorPool();

private:
    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties);

    VKAdapter& m_adapter;
    const vk::PhysicalDevice& m_physical_device;
    uint32_t m_queue_family_index = -1;
    vk::UniqueDevice m_device;
    vk::Queue m_queue;
    vk::UniqueCommandPool m_cmd_pool;
    VKGPUDescriptorPool m_gpu_descriptor_pool;
    const bool m_use_timeline_semaphore = false;
};
