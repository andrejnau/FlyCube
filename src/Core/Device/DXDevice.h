#pragma once
#include "Device/Device.h"
#include <CPUDescriptorPool/DXCPUDescriptorPool.h>
#include <GPUDescriptorPool/DXGPUDescriptorPool.h>
#include <dxgi.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXAdapter;

class DXDevice : public Device
{
public:
    DXDevice(DXAdapter& adapter);
    uint32_t GetTextureDataPitchAlignment() const override;
    std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync) override;
    std::shared_ptr<CommandList> CreateCommandList() override;
    std::shared_ptr<Fence> CreateFence() override;
    std::shared_ptr<Semaphore> CreateGPUSemaphore() override;
    std::shared_ptr<Resource> CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth, int mip_levels) override;
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
    void Wait(const std::shared_ptr<Semaphore>& semaphore) override;
    uint32_t GetShadingRateImageTileSize() const override;
    void Signal(const std::shared_ptr<Semaphore>& semaphore) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence) override;

    DXAdapter& GetAdapter();
    ComPtr<ID3D12Device> GetDevice();
    ComPtr<ID3D12CommandQueue> GetCommandQueue();
    DXCPUDescriptorPool& GetCPUDescriptorPool();
    DXGPUDescriptorPool& GetGPUDescriptorPool();
    bool IsRenderPassesSupported() const;
    bool IsUnderGraphicsDebugger() const;

private:
    std::shared_ptr<Resource> CreateAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs);

    DXAdapter& m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Device5> m_device5;
    ComPtr<ID3D12CommandQueue> m_command_queue;
    DXCPUDescriptorPool m_cpu_descriptor_pool;
    DXGPUDescriptorPool m_gpu_descriptor_pool;
    bool m_is_dxr_supported = false;
    bool m_is_render_passes_supported = false;
    bool m_is_variable_rate_shading_supported = false;
    uint32_t m_shading_rate_image_tile_size = 0;
    bool m_is_under_graphics_debugger = false;
};

D3D12_RESOURCE_STATES ConvertSate(ResourceState state);
D3D12_RAYTRACING_GEOMETRY_DESC FillRaytracingGeometryDesc(const BufferDesc& vertex, const BufferDesc& index);
