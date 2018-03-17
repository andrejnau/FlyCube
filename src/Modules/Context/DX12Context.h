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

    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) override;

    virtual void SetViewport(int width, int height) override;
    virtual void SetScissorRect(LONG left, LONG top, LONG right, LONG bottom) override;

    virtual void IASetIndexBuffer(Resource::Ptr res, UINT SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(UINT slot, Resource::Ptr res, UINT SizeInBytes, UINT Stride) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    virtual void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) override;
    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;

    virtual Resource::Ptr GetBackBuffer() override;
    virtual void Present(const Resource::Ptr& ires) override;

    void ResourceBarrier(const DX12Resource::Ptr& res, D3D12_RESOURCE_STATES state);
    void UseProgram(DX12ProgramApi& program_api);

    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12Device> device;
    std::unique_ptr<DescriptorPool> descriptor_pool;

private:
    virtual void ResizeBackBuffer(int width, int height) override;
    void MoveToNextFrame();

    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fence_event = nullptr;
    uint64_t m_fence_values[FrameCount] = {};
    ComPtr<IDXGISwapChain3> m_swap_chain;
    ComPtr<ID3D12CommandQueue> m_command_queue;
    ComPtr<ID3D12CommandAllocator> m_command_allocator[FrameCount];

    DX12ProgramApi* m_current_program = nullptr;
    std::vector<std::reference_wrapper<DX12ProgramApi>> m_created_program;
};
