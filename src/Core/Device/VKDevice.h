#pragma once
#include "Device/Device.h"
#include <Utilities/Vulkan.h>
#include <GPUDescriptorPool/VKGPUDescriptorPool.h>
#include <GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h>

class VKAdapter;

class VKDevice : public Device
{
public:
    VKDevice(VKAdapter& adapter);
    uint32_t GetTextureDataPitchAlignment() const override;
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList() override;
    std::shared_ptr<Fence> CreateFence(uint64_t initial_value) override;
    std::shared_ptr<Semaphore> CreateGPUSemaphore() override;
    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels) override;
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, MemoryType memory_type) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<Framebuffer> CreateFramebuffer(const std::shared_ptr<Pipeline>& pipeline, uint32_t width, uint32_t height,
                                                   const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) override;
    std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;
    std::shared_ptr<Resource> CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index) override;
    std::shared_ptr<Resource> CreateTopLevelAS(uint32_t instance_count) override;
    bool IsDxrSupported() const override;
    bool IsVariableRateShadingSupported() const override;
    uint32_t GetShadingRateImageTileSize() const override;
    void Wait(const std::shared_ptr<Semaphore>& semaphore) override;
    void Signal(const std::shared_ptr<Semaphore>& semaphore) override;
    void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

    VKAdapter& GetAdapter();
    uint32_t GetQueueFamilyIndex();
    vk::Device GetDevice();
    vk::Queue GetQueue();
    vk::CommandPool GetCmdPool();
    vk::ImageAspectFlags GetAspectFlags(vk::Format format) const;
    VKGPUBindlessDescriptorPoolTyped& GetGPUBindlessDescriptorPool(vk::DescriptorType type);
    VKGPUDescriptorPool& GetGPUDescriptorPool();
    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties);

private:
    std::shared_ptr<Resource> CreateAccelerationStructure(const vk::AccelerationStructureInfoNV& acceleration_structure_info);

    VKAdapter& m_adapter;
    const vk::PhysicalDevice& m_physical_device;
    uint32_t m_queue_family_index = -1;
    vk::UniqueDevice m_device;
    vk::Queue m_queue;
    vk::UniqueCommandPool m_cmd_pool;
    std::map<vk::DescriptorType, VKGPUBindlessDescriptorPoolTyped> m_gpu_bindless_descriptor_pool;
    VKGPUDescriptorPool m_gpu_descriptor_pool;
    bool m_is_variable_rate_shading_supported = false;
    uint32_t m_shading_rate_image_tile_size = 0;
    bool m_is_dxr_supported = false;
};

vk::GeometryNV FillRaytracingGeometryDesc(const BufferDesc& vertex, const BufferDesc& index);
