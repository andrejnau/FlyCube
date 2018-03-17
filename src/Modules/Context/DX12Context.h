#pragma once

#include "Context/Context.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <d3d12.h>
#include <d3dx12.h>
#include "Context/DescriptorPool.h"

using namespace Microsoft::WRL;

class DX12ProgramApi;

class DX12Context : public Context
{
public:
    DX12Context(GLFWwindow* window, int width, int height);

    virtual size_t GetFrameIndex() override
    {
        return frame_index;
    }

    virtual Resource::Ptr CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth = 1, int mip_levels = 1) override;
    virtual Resource::Ptr CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride) override;
    virtual void UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) override;

    virtual Resource::Ptr GetBackBuffer() override;
    void MoveToNextFrame();
    virtual void Present(const Resource::Ptr& ires) override;
    virtual void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) override;

    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;
    virtual void SetViewport(int width, int height) override;
    virtual void SetScissorRect(LONG left, LONG top, LONG right, LONG bottom) override;
    virtual void OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv) override;
    virtual void ClearRenderTarget(Resource::Ptr rtv, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(Resource::Ptr dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual std::unique_ptr<ProgramApi> CreateProgram() override;

    virtual void IASetIndexBuffer(Resource::Ptr res, UINT SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(UINT slot, Resource::Ptr res, UINT SizeInBytes, UINT Stride) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12Fence> m_fence;
    uint32_t m_fenceValues[FrameCount] = {};
    uint32_t frame_index = 0;
    HANDLE m_fenceEvent;

    void ResourceBarrier(const DX12Resource::Ptr& res, D3D12_RESOURCE_STATES state)
    {
        if (res->state != state)
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res->default_res.Get(), res->state, state));
        res->state = state;
    }

    ComPtr<ID3D12GraphicsCommandList> commandList;

    ComPtr<ID3D12CommandAllocator> commandAllocator[FrameCount];
    DX12ProgramApi* current_program = nullptr;

    std::unique_ptr<DescriptorPool> descriptor_pool;
private:
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<IDXGISwapChain3> swap_chain;
};
