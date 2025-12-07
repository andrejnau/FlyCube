#pragma once
#include "Device/Device.h"
#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"
#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include <vulkan/vulkan.hpp>

class VKAdapter;
class VKCommandQueue;

struct InlineUniformBlockProperties {
    uint32_t max_total_size;
    uint32_t max_block_size;
    uint32_t max_per_stage_blocks;
    uint32_t max_blocks;
};

vk::ImageLayout ConvertState(ResourceState state);
vk::BuildAccelerationStructureFlagsKHR Convert(BuildAccelerationStructureFlags flags);
vk::Extent2D ConvertShadingRate(ShadingRate shading_rate);
std::array<vk::FragmentShadingRateCombinerOpKHR, 2> ConvertShadingRateCombiners(
    const std::array<ShadingRateCombiner, 2>& combiners);

vk::CompareOp ConvertToCompareOp(ComparisonFunc func);

class VKDevice : public Device {
public:
    explicit VKDevice(VKAdapter& adapter);
    std::shared_ptr<Memory> AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits) override;
    std::shared_ptr<CommandQueue> GetCommandQueue(CommandListType type) override;
    uint32_t GetTextureDataPitchAlignment() const override;
    std::shared_ptr<Swapchain> CreateSwapchain(const NativeSurface& surface,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t frame_count,
                                               bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList(CommandListType type) override;
    std::shared_ptr<Fence> CreateFence(uint64_t initial_value) override;
    MemoryRequirements GetTextureMemoryRequirements(const TextureDesc& desc) override;
    MemoryRequirements GetMemoryBufferRequirements(const BufferDesc& desc) override;
    std::shared_ptr<Resource> CreatePlacedTexture(const std::shared_ptr<Memory>& memory,
                                                  uint64_t offset,
                                                  const TextureDesc& desc) override;
    std::shared_ptr<Resource> CreatePlacedBuffer(const std::shared_ptr<Memory>& memory,
                                                 uint64_t offset,
                                                 const BufferDesc& desc) override;
    std::shared_ptr<Resource> CreateTexture(MemoryType memory_type, const TextureDesc& desc) override;
    std::shared_ptr<Resource> CreateBuffer(MemoryType memory_type, const BufferDesc& desc) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<BindingSetLayout> CreateBindingSetLayout(const BindingSetLayoutDesc& desc) override;
    std::shared_ptr<BindingSet> CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout) override;
    std::shared_ptr<Shader> CreateShader(const std::vector<uint8_t>& blob,
                                         ShaderBlobType blob_type,
                                         ShaderType shader_type) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;
    std::shared_ptr<Resource> CreateAccelerationStructure(const AccelerationStructureDesc& desc) override;
    std::shared_ptr<QueryHeap> CreateQueryHeap(QueryHeapType type, uint32_t count) override;
    bool IsDxrSupported() const override;
    bool IsRayQuerySupported() const override;
    bool IsVariableRateShadingSupported() const override;
    bool IsMeshShadingSupported() const override;
    bool IsDrawIndirectCountSupported() const override;
    bool IsGeometryShaderSupported() const override;
    bool IsBindlessSupported() const override;
    bool IsSamplerFilterMinmaxSupported() const override;
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
    uint64_t GetConstantBufferOffsetAlignment() const override;

    VKAdapter& GetAdapter();
    vk::Device GetDevice();
    CommandListType GetAvailableCommandListType(CommandListType type);
    vk::CommandPool GetCmdPool(CommandListType type);
    vk::ImageAspectFlags GetAspectFlags(vk::Format format) const;
    VKGPUBindlessDescriptorPoolTyped& GetGPUBindlessDescriptorPool(vk::DescriptorType type);
    VKGPUDescriptorPool& GetGPUDescriptorPool();
    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties);
    vk::AccelerationStructureGeometryKHR FillRaytracingGeometryTriangles(const RaytracingGeometryBufferDesc& vertex,
                                                                         const RaytracingGeometryBufferDesc& index,
                                                                         RaytracingGeometryFlags flags) const;

    uint32_t GetMaxDescriptorSetBindings(vk::DescriptorType type) const;
    bool HasBufferDeviceAddress() const;
    bool IsInlineUniformBlockSupported() const;
    const InlineUniformBlockProperties& GetInlineUniformBlockProperties() const;

    template <typename Features>
    Features GetFeatures2() const
    {
        auto [_, features] = physical_device_.getFeatures2<vk::PhysicalDeviceFeatures2, Features>();
        return features;
    }

    template <typename Properties>
    Properties GetProperties2() const
    {
        auto [_, properties] = physical_device_.getProperties2<vk::PhysicalDeviceProperties2, Properties>();
        return properties;
    }

    template <typename Properties>
    Properties GetMemoryProperties2() const
    {
        auto [_, properties] = physical_device_.getMemoryProperties2<vk::PhysicalDeviceMemoryProperties2, Properties>();
        return properties;
    }

private:
    RaytracingASPrebuildInfo GetAccelerationStructurePrebuildInfo(
        const vk::AccelerationStructureBuildGeometryInfoKHR& acceleration_structure_info,
        const std::vector<uint32_t>& max_primitive_counts) const;

    VKAdapter& adapter_;
    const vk::PhysicalDevice& physical_device_;
    vk::UniqueDevice device_;
    struct QueueInfo {
        uint32_t queue_family_index;
        uint32_t queue_count;
    };
    std::map<CommandListType, QueueInfo> queues_info_;
    std::map<CommandListType, vk::UniqueCommandPool> cmd_pools_;
    std::map<CommandListType, std::shared_ptr<VKCommandQueue>> command_queues_;
    std::map<vk::DescriptorType, VKGPUBindlessDescriptorPoolTyped> gpu_bindless_descriptor_pool_;
    VKGPUDescriptorPool gpu_descriptor_pool_;
    bool is_variable_rate_shading_supported_ = false;
    uint32_t shading_rate_image_tile_size_ = 0;
    bool is_dxr_supported_ = false;
    bool is_ray_query_supported_ = false;
    bool is_mesh_shading_supported_ = false;
    uint32_t shader_group_handle_size_ = 0;
    uint32_t shader_record_alignment_ = 0;
    uint32_t shader_table_alignment_ = 0;
    bool geometry_shader_supported_ = false;
    bool bindless_supported_ = false;
    bool sampler_filter_minmax_supported_ = false;
    bool draw_indirect_count_supported_ = false;
    bool has_buffer_device_address_ = false;
    bool inline_uniform_block_supported_ = false;
    InlineUniformBlockProperties inline_uniform_block_properties_;
    vk::PhysicalDeviceProperties device_properties_ = {};
};
