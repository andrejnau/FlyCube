#include "Context/Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Context::Context(GLFWwindow* window, int width, int height)
    : window(window)
{
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
    bufferDesc.Width = width;
    bufferDesc.Height = height;
    bufferDesc.RefreshRate.Numerator = 60;
    bufferDesc.RefreshRate.Denominator = 1;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = glfwGetWin32Window(window);
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    DWORD create_device_flags = 0;
#if 1
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ASSERT_SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, nullptr, 0,
        D3D11_SDK_VERSION, &swapChainDesc, &swap_chain, &device, nullptr, &device_context));

    device_context.As(&perf);
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
