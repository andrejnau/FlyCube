#pragma once

#include "Context/Context.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <DXGI1_4.h>
#include <wrl.h>
#include <GLFW/glfw3.h>
#include <vector>

using namespace Microsoft::WRL;

class DX12Context : public Context
{
public:
    DX12Context(GLFWwindow* window, int width, int height);
    virtual ComPtr<IUnknown> GetBackBuffer() override;
    virtual void Present() override;
    virtual void DrawIndexed(UINT IndexCount) override;

    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12Fence> m_fence;
    uint32_t m_fenceValue = 0;
    uint32_t frame_index = 0;
    HANDLE m_fenceEvent;

private:
    void WaitForPreviousFrame();
    virtual void ResizeBackBuffer(int width, int height) override;
    ComPtr<IDXGISwapChain3> swap_chain;
};
