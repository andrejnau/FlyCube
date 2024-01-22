#pragma once
#include "Device/Device.h"
#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"
#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include <vulkan/vulkan.hpp>

class VKAdapter;
class VKCommandQueue;

vk::ImageLayout ConvertState(ResourceState state);
vk::BuildAccelerationStructureFlagsKHR Convert(BuildAccelerationStructureFlags flags);

class VKDevice : public Device {
public:
    VKDevice(VKAdapter& adapter);
    std::shared_ptr<Memory> AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits) override;
    std::shared_ptr<CommandQueue> GetCommandQueue(CommandListType type) override;
    uint32_t GetTextureDataPitchAlignment() const override;
    std::shared_ptr<Swapchain> CreateSwapchain(WindowHandle window,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t frame_count,
                                               bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList(CommandListType type) override;
    std::shared_ptr<Fence> CreateFence(uint64_t initial_value) override;
    std::shared_ptr<Resource> CreateTexture(TextureType type,
                                            uint32_t bind_flag,
                                            gli::format format,
                                            uint32_t sample_count,
                                            int width,
                                            int height,
                                            int depth,
                                            int mip_levels) override;
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<BindingSetLayout> CreateBindingSetLayout(const std::vector<BindKey>& descs) override;
    std::shared_ptr<BindingSet> CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout) override;
    std::shared_ptr<RenderPass> CreateRenderPass(const RenderPassDesc& desc) override;
    std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc& desc) override;
    std::shared_ptr<Shader> CreateShader(const std::vector<uint8_t>& blob,
                                         ShaderBlobType blob_type,
                                         ShaderType shader_type) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) override;
    std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;
    std::shared_ptr<Resource> CreateAccelerationStructure(AccelerationStructureType type,
                                                          const std::shared_ptr<Resource>& resource,
                                                          uint64_t offset) override;
    std::shared_ptr<QueryHeap> CreateQueryHeap(QueryHeapType type, uint32_t count) override;
    bool IsDxrSupported() const override;
    bool IsRayQuerySupported() const override;
    bool IsVariableRateShadingSupported() const override;
    bool IsMeshShadingSupported() const override;
    bool IsDrawIndirectCountSupported() const override;
    bool IsGeometryShaderSupported() const override;
    uint32_t GetShadingRateImageTileSize() const override;
    MemoryBudget GetMemoryBudget() const override;
    uint32_t GetShaderGroupHandleSize() const override;
    uint32_t GetShaderRecordAlignment() const override;
    uint32_t GetShaderTableAlignment() const override;
    RaytracingASPrebuildInfo GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs,
                                                 BuildAccelerationStructureFlags flags) const override;
    RaytracingASPrebuildInfo GetTLASPrebuildInfo(uint32_t instance_count,
                                                 BuildAccelerationStructureFlags flags) const override;
    ShaderBlobType GetSupportedShaderBlobType() const override;

    VKAdapter& GetAdapter();
    vk::Device GetDevice();
    CommandListType GetAvailableCommandListType(CommandListType type);
    vk::CommandPool GetCmdPool(CommandListType type);
    vk::ImageAspectFlags GetAspectFlags(vk::Format format) const;
    VKGPUBindlessDescriptorPoolTyped& GetGPUBindlessDescriptorPool(vk::DescriptorType type);
    VKGPUDescriptorPool& GetGPUDescriptorPool();
    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties);
    vk::AccelerationStructureGeometryKHR FillRaytracingGeometryTriangles(const BufferDesc& vertex,
                                                                         const BufferDesc& index,
                                                                         RaytracingGeometryFlags flags) const;

    uint32_t GetMaxDescriptorSetBindings(vk::DescriptorType type) const;

private:
    RaytracingASPrebuildInfo GetAccelerationStructurePrebuildInfo(
        const vk::AccelerationStructureBuildGeometryInfoKHR& acceleration_structure_info,
        const std::vector<uint32_t>& max_primitive_counts) const;

    VKAdapter& m_adapter;
    const vk::PhysicalDevice& m_physical_device;
    vk::UniqueDevice m_device;
    struct QueueInfo {
        uint32_t queue_family_index;
        uint32_t queue_count;
    };
    std::map<CommandListType, QueueInfo> m_queues_info;
    std::map<CommandListType, vk::UniqueCommandPool> m_cmd_pools;
    std::map<CommandListType, std::shared_ptr<VKCommandQueue>> m_command_queues;
    std::map<vk::DescriptorType, VKGPUBindlessDescriptorPoolTyped> m_gpu_bindless_descriptor_pool;
    VKGPUDescriptorPool m_gpu_descriptor_pool;
    bool m_is_variable_rate_shading_supported = false;
    uint32_t m_shading_rate_image_tile_size = 0;
    bool m_is_dxr_supported = false;
    bool m_is_ray_query_supported = false;
    bool m_is_mesh_shading_supported = false;
    uint32_t m_shader_group_handle_size = 0;
    uint32_t m_shader_record_alignment = 0;
    uint32_t m_shader_table_alignment = 0;
    bool m_draw_indirect_count_supported = false;
    bool m_geometry_shader_supported = false;
    vk::PhysicalDeviceProperties m_device_properties = {};
};
