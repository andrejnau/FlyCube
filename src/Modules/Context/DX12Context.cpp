#include "Context/DX12Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <d3d12.h>

DX12Context::DX12Context(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    ComPtr<IDXGIFactory4> dxgi_factory;
    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

    auto GetHardwareAdapter = [&](ComPtr<IDXGIFactory4> dxgiFactory) -> ComPtr<IDXGIAdapter1>
    {
        ComPtr<IDXGIAdapter1> adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, _uuidof(ID3D12Device), nullptr)))
                return adapter;
        }
        return nullptr;
    };

    ComPtr<IDXGIAdapter1> adapter = GetHardwareAdapter(dxgi_factory.Get());
    ASSERT(adapter != nullptr);
    ASSERT_SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device)));

    D3D12_COMMAND_QUEUE_DESC cqDesc = {}; // we will be using all the default values
    ASSERT_SUCCEEDED(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue))); // create the command queue

    ASSERT_SUCCEEDED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_factory)));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 5;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> tmp_swap_chain;
    ASSERT_SUCCEEDED(dxgi_factory->CreateSwapChainForHwnd(device.Get(), glfwGetWin32Window(window), &swap_chain_desc, nullptr, nullptr, &tmp_swap_chain));
    tmp_swap_chain.As(&swap_chain);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ASSERT_SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ASSERT_SUCCEEDED(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

ComPtr<IUnknown> DX12Context::GetBackBuffer()
{
    ComPtr<ID3D12Resource> back_buffer;
    ASSERT_SUCCEEDED(swap_chain->GetBuffer(frame_index, IID_PPV_ARGS(&back_buffer)));
    return back_buffer;
}

void DX12Context::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ASSERT_SUCCEEDED(commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ASSERT_SUCCEEDED(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    frame_index = swap_chain->GetCurrentBackBufferIndex();
}


void DX12Context::Present()
{
    ASSERT_SUCCEEDED(swap_chain->Present(1, 0));

    WaitForPreviousFrame();
}

void DX12Context::DrawIndexed(UINT IndexCount)
{
}

void DX12Context::ResizeBackBuffer(int width, int height)
{
}
