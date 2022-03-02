#pragma once
#include "Device/Device.h"
#import <Metal/Metal.h>

class MTDevice : public Device
{
public:
    MTDevice(const id<MTLDevice>& device);
    std::shared_ptr<Memory> AllocateMemory(uint64_t size, MemoryType memory_type, uint32_t memory_type_bits) override;
    std::shared_ptr<CommandQueue> GetCommandQueue(CommandListType type) override;
    uint32_t GetTextureDataPitchAlignment() const override;
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList(CommandListType type) override;
    std::shared_ptr<Fence> CreateFence(uint64_t initial_value) override;
    std::shared_ptr<Resource> CreateTexture(TextureType type, uint32_t bind_flag, gli::format format, uint32_t sample_count, int width, int height, int depth, int mip_levels) override;
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, uint32_t buffer_size) override;
    std::shared_ptr<Resource> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<View> CreateView(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc) override;
    std::shared_ptr<BindingSetLayout> CreateBindingSetLayout(const std::vector<BindKey>& descs) override;
    std::shared_ptr<BindingSet> CreateBindingSet(const std::shared_ptr<BindingSetLayout>& layout) override;
    std::shared_ptr<RenderPass> CreateRenderPass(const RenderPassDesc& desc) override;
    std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc& desc) override;
    std::shared_ptr<Shader> CompileShader(const ShaderDesc& desc) override;
    std::shared_ptr<Program> CreateProgram(const std::vector<std::shared_ptr<Shader>>& shaders) override;
    std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::shared_ptr<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) override;
    std::shared_ptr<Resource> CreateAccelerationStructure(AccelerationStructureType type, const std::shared_ptr<Resource>& resource, uint64_t offset) override;
    std::shared_ptr<QueryHeap> CreateQueryHeap(QueryHeapType type, uint32_t count) override;
    bool IsDxrSupported() const override;
    bool IsRayQuerySupported() const override;
    bool IsVariableRateShadingSupported() const override;
    bool IsMeshShadingSupported() const override;
    uint32_t GetShadingRateImageTileSize() const override;
    MemoryBudget GetMemoryBudget() const override;
    uint32_t GetShaderGroupHandleSize() const override;
    uint32_t GetShaderRecordAlignment() const override;
    uint32_t GetShaderTableAlignment() const override;
    RaytracingASPrebuildInfo GetBLASPrebuildInfo(const std::vector<RaytracingGeometryDesc>& descs, BuildAccelerationStructureFlags flags) const override;
    RaytracingASPrebuildInfo GetTLASPrebuildInfo(uint32_t instance_count, BuildAccelerationStructureFlags flags) const override;
    
private:
    id<MTLDevice> m_device;
};
