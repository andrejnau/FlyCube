#include "Context/Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Context::Context(GLFWwindow* window, int width, int height)
    : window(window)
{
    DWORD create_device_flags = 0;
#if 1
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ASSERT_SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, nullptr, 0,
        D3D11_SDK_VERSION, &device, nullptr, &device_context));

    device_context.As(&perf);

    ComPtr<IDXGIFactory4> dxgi_factory;
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
}

ComPtr<ID3D11Resource> Context::GetBackBuffer()
{
    ComPtr<ID3D11Texture2D> back_buffer;
    ASSERT_SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));
    return back_buffer;
}

void Context::ResizeBackBuffer(int width, int height)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    ASSERT_SUCCEEDED(swap_chain->GetDesc(&desc));
    ASSERT_SUCCEEDED(swap_chain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags));
}

void Context::Present()
{
    ASSERT_SUCCEEDED(swap_chain->Present(0, 0));
}

void Context::DrawIndexed(UINT IndexCount)
{
    device_context->DrawIndexed(IndexCount, 0, 0);
}
