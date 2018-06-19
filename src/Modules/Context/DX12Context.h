#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <vector>

#include "Context/Context.h"
#include "Context/DescriptorPool.h"
#include "Context/DX12Resource.h"

using namespace Microsoft::WRL;

class DX12ProgramApi;

class DX12Context : public Context
{
public:
    DX12Context(GLFWwindow* window, int width, int height);
    ~DX12Context();

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void *pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch) override;

    virtual void SetViewport(float width, float height) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, uint32_t SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(uint32_t slot, Resource::Ptr res, uint32_t SizeInBytes, uint32_t Stride) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) override;
    virtual void Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;

    void InitBackBuffers();

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present(const Resource::Ptr& ires) override;

    virtual void OnDestroy() override;

    void ResourceBarrier(const DX12Resource::Ptr& res, D3D12_RESOURCE_STATES state);
    void ResourceBarrier(DX12Resource& res, D3D12_RESOURCE_STATES state);
    void UseProgram(DX12ProgramApi& program_api);

    DescriptorPool& GetDescriptorPool();
    void QueryOnDelete(ComPtr<IUnknown> res);

    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12Device> device;
    std::unique_ptr<DescriptorPool> descriptor_pool[FrameCount];

private:
    virtual void ResizeBackBuffer(int width, int height) override;
    void WaitForGpu();
    void MoveToNextFrame();

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
};
