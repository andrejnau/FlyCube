#pragma once
#include "CPUDescriptorPool/DXCPUDescriptorPool.h"
#include "Device/Device.h"
#include "GPUDescriptorPool/DXGPUDescriptorPool.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXAdapter;
class DXCommandQueue;

D3D12_RESOURCE_STATES ConvertState(ResourceState state);
D3D12_HEAP_TYPE GetHeapType(MemoryType memory_type);
D3D12_RAYTRACING_GEOMETRY_DESC FillRaytracingGeometryDesc(const RaytracingGeometryBufferDesc& vertex,
                                                          const RaytracingGeometryBufferDesc& index,
                                                          RaytracingGeometryFlags flags);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS Convert(BuildAccelerationStructureFlags flags);

class DXDevice : public Device {
public:
    DXDevice(DXAdapter& adapter);
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
    std::shared_ptr<BindingSetLayout> CreateBindingSetLayout(const std::vector<BindKey>& bind_keys) override;
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

    DXAdapter& GetAdapter();
    ComPtr<ID3D12Device> GetDevice();
    DXCPUDescriptorPool& GetCPUDescriptorPool();
    DXGPUDescriptorPool& GetGPUDescriptorPool();
    bool IsUnderGraphicsDebugger() const;
    bool IsCreateNotZeroedAvailable() const;
    ID3D12CommandSignature* GetCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride);

private:
    RaytracingASPrebuildInfo GetAccelerationStructurePrebuildInfo(
        const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const;

    DXAdapter& adapter_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12Device5> device5_;
    std::map<CommandListType, std::shared_ptr<DXCommandQueue>> command_queues_;
    DXCPUDescriptorPool cpu_descriptor_pool_;
    DXGPUDescriptorPool gpu_descriptor_pool_;
    bool is_dxr_supported_ = false;
    bool is_ray_query_supported_ = false;
    bool is_variable_rate_shading_supported_ = false;
    bool is_mesh_shading_supported_ = false;
    uint32_t shading_rate_image_tile_size_ = 0;
    bool is_under_graphics_debugger_ = false;
    bool is_create_not_zeroed_available_ = false;
    std::map<std::pair<D3D12_INDIRECT_ARGUMENT_TYPE, uint32_t>, ComPtr<ID3D12CommandSignature>>
        command_signature_cache_;
};
