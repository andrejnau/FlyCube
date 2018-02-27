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

using namespace Microsoft::WRL;

// {975B960E-21B5-4FD4-8301-53914554BDCA}
static const GUID buffer_stride =
{ 0x975b960e, 0x21b5, 0x4fd4,{ 0x83, 0x1, 0x53, 0x91, 0x45, 0x54, 0xbd, 0xca } };

// {D3B925BB-7647-4A75-8705-45F3ED5A71CB}
static const GUID buffer_bind_flag =
{ 0xd3b925bb, 0x7647, 0x4a75,{ 0x87, 0x5, 0x45, 0xf3, 0xed, 0x5a, 0x71, 0xcb } };

class DX12ProgramApi;

class DX12Context : public Context
{
public:
    DX12Context(GLFWwindow* window, int width, int height);
    virtual ComPtr<IUnknown> GetBackBuffer() override;
    virtual void Present() override;
    virtual void DrawIndexed(UINT IndexCount) override;

    virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;
    virtual void SetViewport(int width, int height) override;
    virtual void OMSetRenderTargets(std::vector<ComPtr<IUnknown>> rtv, ComPtr<IUnknown> dsv) override;
    virtual void ClearRenderTarget(ComPtr<IUnknown> rtv, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(ComPtr<IUnknown> dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual ComPtr<IUnknown> CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels) override;
    virtual ComPtr<IUnknown> CreateSamplerAnisotropic() override;
    virtual ComPtr<IUnknown> CreateSamplerShadow() override;
    virtual ComPtr<IUnknown> CreateShadowRSState() override;
    virtual void RSSetState(ComPtr<IUnknown> state) override;
    virtual std::unique_ptr<ProgramApi> CreateProgram() override;
    virtual ComPtr<IUnknown> CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride, const std::string& name) override;

    virtual void IASetIndexBuffer(ComPtr<IUnknown> res, UINT SizeInBytes, DXGI_FORMAT Format) override;
    virtual void IASetVertexBuffer(UINT slot, ComPtr<IUnknown> res, UINT SizeInBytes, UINT Stride) override;

    virtual void UpdateSubresource(ComPtr<IUnknown> ires, UINT DstSubresource, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) override;

    virtual void BeginEvent(LPCWSTR Name) override;
    virtual void EndEvent() override;

    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12Fence> m_fence;
    uint32_t m_fenceValue = 0;
    uint32_t frame_index = 0;
    HANDLE m_fenceEvent;


    static const int frameBufferCount = 2; // number of buffers we want, 2 for double buffering, 3 for tripple buffering
    ComPtr<ID3D12GraphicsCommandList> commandList; // a command list we can record commands into, then execute them to render the frame
    ComPtr<ID3D12CommandAllocator> commandAllocator; // we want enough a
    DX12ProgramApi* current_program = nullptr;
private:
    void WaitForPreviousFrame();
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<IDXGISwapChain3> swap_chain;
};
