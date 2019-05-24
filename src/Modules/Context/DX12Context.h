#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <vector>

#include "Context/Context.h"
#include "Context/DescriptorPool.h"
#include <Resource/DX12Resource.h>

using namespace Microsoft::WRL;

class DX12ProgramApi;

class DX12Context : public Context
{
public:
    DX12Context(GLFWwindow* window);
    ~DX12Context();

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, gli::format format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual Resource::Ptr CreateSampler(const SamplerDesc& desc) override;
    virtual Resource::Ptr CreateBottomLevelAS(const BufferDesc& vertex) override;
    virtual Resource::Ptr CreateBottomLevelAS(const BufferDesc& vertex, const BufferDesc& index) override;
    virtual Resource::Ptr CreateTopLevelAS(const std::vector<std::pair<Resource::Ptr, glm::mat4>>& geometry) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) override;

    virtual void SetViewport(float width, float height) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, gli::format Format) override;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res) override;

    virtual void BeginEvent(const std::string& name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) override;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;
    virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) override;

    void InitBackBuffers();

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present() override;

    virtual bool IsDxrSupported() const override { return m_is_dxr_supported; }

    virtual void OnDestroy() override;

    void ResourceBarrier(const DX12Resource::Ptr& res, D3D12_RESOURCE_STATES state);
    void ResourceBarrier(DX12Resource& res, D3D12_RESOURCE_STATES state);
    void UseProgram(DX12ProgramApi& program_api);

    DescriptorPool& GetDescriptorPool();
    void QueryOnDelete(ComPtr<IUnknown> res);

    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12GraphicsCommandList4> command_list4;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12Device5> device5;
    std::unique_ptr<DescriptorPool> descriptor_pool;

    DX12ViewPool m_view_pool;
    bool m_use_render_passes = true;
    bool m_is_open_render_pass = false;

private:
    virtual void ResizeBackBuffer(int width, int height) override;
    void WaitForGpu();
    void MoveToNextFrame();
    void CloseRenderPass();

    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fence_event = nullptr;
    uint64_t m_fence_values[FrameCount] = {};
    ComPtr<IDXGISwapChain3> m_swap_chain;
    ComPtr<ID3D12CommandQueue> m_command_queue;
    ComPtr<ID3D12CommandAllocator> m_command_allocator[FrameCount];
    std::vector<ComPtr<IUnknown>> m_deletion_queue[FrameCount];

    DX12ProgramApi* m_current_program = nullptr;
    std::vector<std::reference_wrapper<DX12ProgramApi>> m_created_program;
    Resource::Ptr m_back_buffers[FrameCount];
    bool m_is_dxr_supported = false;
};
