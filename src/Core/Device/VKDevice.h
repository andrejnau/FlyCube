#pragma once
#include "Device/Device.h"
#include <vulkan/vulkan.hpp>
#include <GPUDescriptorPool/VKGPUDescriptorPool.h>
#include <GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h>

class VKAdapter;
class VKCommandQueue;

class VKDevice : public Device
{
public:
    VKDevice(VKAdapter& adapter);
    std::shared_ptr<Memory> AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits) override;
    std::shared_ptr<CommandQueue> GetCommandQueue(CommandListType type) override;
    uint32_t GetTextureDataPitchAlignment() const override;
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList(CommandListType type) override;
    std::shared_ptr<Fence> CreateFence(uint64_t initial_value) override;
    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels) override;
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<BindingSetLayout> CreateBindingSetLayout(const std::vector<BindKey>& descs) override;
    std::shared_ptr<BindingSet> CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout) override;
    std::shared_ptr<RenderPass> CreateRenderPass(const RenderPassDesc& desc) override;
    std::shared_ptr<Framebuffer> CreateFramebuffer(const std::shared_ptr<RenderPass>& render_pass, uint32_t width, uint32_t height,
                                                   const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) override;
    std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;
    std::shared_ptr<Resource> CreateBottomLevelAS(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags) override;
    std::shared_ptr<Resource> CreateTopLevelAS(uint32_t instance_count, BuildAccelerationStructureFlags flags) override;
    bool IsDxrSupported() const override;
    bool IsVariableRateShadingSupported() const override;
    bool IsMeshShadingSupported() const override;
    uint32_t GetShadingRateImageTileSize() const override;
    MemoryBudget GetMemoryBudget() const override;
    uint32_t GetShaderGroupHandleSize() const override;
    uint32_t GetShaderRecordAlignment() const override;
    uint32_t GetShaderTableAlignment() const override;

    VKAdapter& GetAdapter();
    vk::Device GetDevice();
    vk::CommandPool GetCmdPool(CommandListType type);
    vk::ImageAspectFlags GetAspectFlags(vk::Format format) const;
    VKGPUBindlessDescriptorPoolTyped& GetGPUBindlessDescriptorPool(vk::DescriptorType type);
    VKGPUDescriptorPool& GetGPUDescriptorPool();
    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties);
    vk::AccelerationStructureGeometryKHR FillRaytracingGeometryTriangles(const BufferDesc& vertex, const BufferDesc& index, RaytracingGeometryFlags flags);

private:
    std::shared_ptr<Resource> CreateAccelerationStructure(const vk::AccelerationStructureBuildGeometryInfoKHR& acceleration_structure_info, const std::vector<uint32_t>& max_primitive_counts);

    VKAdapter& m_adapter;
    const vk::PhysicalDevice& m_physical_device;
    vk::UniqueDevice m_device;
    struct PerQueueData
    {
        uint32_t queue_family_index = -1;
        vk::UniqueCommandPool cmd_pool;
    };
    std::map<CommandListType, PerQueueData> m_per_queue_data;
    std::map<CommandListType, std::shared_ptr<VKCommandQueue>> m_command_queues;
    std::map<vk::DescriptorType, VKGPUBindlessDescriptorPoolTyped> m_gpu_bindless_descriptor_pool;
    VKGPUDescriptorPool m_gpu_descriptor_pool;
    bool m_is_variable_rate_shading_supported = false;
    uint32_t m_shading_rate_image_tile_size = 0;
    bool m_is_dxr_supported = false;
    bool m_is_mesh_shading_supported = false;
    uint32_t m_shader_group_handle_size = 0;
    uint32_t m_shader_record_alignment = 0;
    uint32_t m_shader_table_alignment = 0;
};

vk::ImageLayout ConvertState(ResourceState state);