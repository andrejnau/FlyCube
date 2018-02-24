#include "Context/DX11Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dxgiformat.h>

DX11Context::DX11Context(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
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

ComPtr<IUnknown> DX11Context::GetBackBuffer()
{
    ComPtr<ID3D11Texture2D> back_buffer;
    ASSERT_SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));
    return back_buffer;
}

void DX11Context::ResizeBackBuffer(int width, int height)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    ASSERT_SUCCEEDED(swap_chain->GetDesc(&desc));
    ASSERT_SUCCEEDED(swap_chain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags));
}

void DX11Context::Present()
{
    ASSERT_SUCCEEDED(swap_chain->Present(0, 0));
}

void DX11Context::DrawIndexed(UINT IndexCount)
{
    device_context->DrawIndexed(IndexCount, 0, 0);
}

void DX11Context::SetViewport(int width, int height)
{
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    device_context->RSSetViewports(1, &viewport);
}

ComPtr<IUnknown> DX11Context::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth)
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.ArraySize = depth;
    texture_desc.MipLevels = 1;
    texture_desc.Format = format;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;

    if (bind_flag & BindFlag::kRtv)
        texture_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    if (bind_flag & BindFlag::kDsv)
        texture_desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    if (bind_flag & BindFlag::kSrv)
        texture_desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    if (depth > 1)
        texture_desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

    uint32_t quality = 0;
    device->CheckMultisampleQualityLevels(texture_desc.Format, msaa_count, &quality);
    texture_desc.SampleDesc.Count = msaa_count;
    texture_desc.SampleDesc.Quality = quality - 1;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(device->CreateTexture2D(&texture_desc, nullptr, &texture));

    return texture;
}

ComPtr<IUnknown> DX11Context::CreateBufferSRV(void* data, size_t size, size_t stride)
{
    ComPtr<ID3D11Buffer> buffer;
    if (size == 0)
        return buffer;

    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.ByteWidth = size * stride;
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = stride;
    ASSERT_SUCCEEDED(device->CreateBuffer(&buffer_desc, nullptr, &buffer));

    device_context->UpdateSubresource(buffer.Get(), 0, nullptr, data, 0, 0);

    return buffer;
}

ComPtr<IUnknown> DX11Context::CreateSamplerAnisotropic()
{
    ComPtr<ID3D11SamplerState> m_texture_sampler;

    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(device->CreateSamplerState(&samp_desc, &m_texture_sampler));
    return m_texture_sampler;
}

ComPtr<IUnknown> DX11Context::CreateSamplerShadow()
{
    ComPtr<ID3D11SamplerState> m_shadow_sampler;

    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(device->CreateSamplerState(&samp_desc, &m_shadow_sampler));
    return m_shadow_sampler;
}
