#include "Context/DX11Context.h"
#include <Utilities/DXUtility.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dxgiformat.h>
#include <Program/DX11ProgramApi.h>

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

    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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

void DX11Context::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
    device_context->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
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

void DX11Context::OMSetRenderTargets(std::vector<ComPtr<IUnknown>> rtv_res, ComPtr<IUnknown> dsv_res)
{
    std::vector<ComPtr<ID3D11RenderTargetView>> rtv;
    std::vector<ID3D11RenderTargetView*> rtv_ptr;
    for (auto & res : rtv_res)
    {
        rtv.emplace_back(ToRtv(res));
        rtv_ptr.emplace_back(rtv.back().Get());
    }
    ComPtr<ID3D11DepthStencilView> _dsv = ToDsv(dsv_res);
    device_context->OMSetRenderTargets(rtv_ptr.size(), rtv_ptr.data(), _dsv.Get());
}

void DX11Context::ClearRenderTarget(ComPtr<IUnknown> rtv, const FLOAT ColorRGBA[4])
{
    device_context->ClearRenderTargetView(ToRtv(rtv).Get(), ColorRGBA);
}

void DX11Context::ClearDepthStencil(ComPtr<IUnknown> dsv, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    device_context->ClearDepthStencilView(ToDsv(dsv).Get(), ClearFlags, Depth, Stencil);
}

ComPtr<IUnknown> DX11Context::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.ArraySize = depth;
    texture_desc.MipLevels = mip_levels;
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

ComPtr<IUnknown> DX11Context::CreateShadowRSState()
{
    ComPtr<ID3D11RasterizerState> rasterizer_state;
    D3D11_RASTERIZER_DESC shadowState = {};
    ZeroMemory(&shadowState, sizeof(D3D11_RASTERIZER_DESC));
    shadowState.FillMode = D3D11_FILL_SOLID;
    shadowState.CullMode = D3D11_CULL_BACK;
    shadowState.DepthBias = 4096;
    device->CreateRasterizerState(&shadowState, &rasterizer_state);
    return rasterizer_state;
}

void DX11Context::RSSetState(ComPtr<IUnknown> state)
{
    ComPtr<ID3D11RasterizerState> rasterizer_state;
    state.As(&rasterizer_state);
    device_context->RSSetState(rasterizer_state.Get());
}

std::unique_ptr<ProgramApi> DX11Context::CreateProgram()
{
    return std::make_unique<DX11ProgramApi>(*this);
}

ComPtr<IUnknown> DX11Context::CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride, const std::string& name)
{
    ComPtr<ID3D11Buffer> buffer;
    if (buffer_size == 0)
        return buffer;
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = buffer_size;
    desc.StructureByteStride = stride;
    if (stride != 0)
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    if (bind_flag & BindFlag::kUav)
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    if (bind_flag & BindFlag::kCbv)
        desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
    if (bind_flag & BindFlag::kSrv)
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (bind_flag & BindFlag::kVbv)
        desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    if (bind_flag & BindFlag::kIbv)
        desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;

    ASSERT_SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffer));
    buffer->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
    return buffer;
}

void DX11Context::IASetIndexBuffer(ComPtr<IUnknown> res, UINT SizeInBytes, DXGI_FORMAT Format)
{
    ComPtr<ID3D11Buffer> buf;
    res.As(&buf);
    device_context->IASetIndexBuffer(buf.Get(), Format, 0);
}

void DX11Context::IASetVertexBuffer(UINT slot, ComPtr<IUnknown> res, UINT SizeInBytes, UINT Stride)
{
    ComPtr<ID3D11Buffer> buf;
    res.As(&buf);
    UINT offset = 0;
    device_context->IASetVertexBuffers(slot, 1, buf.GetAddressOf(), &Stride, &offset);
}

void DX11Context::UpdateSubresource(ComPtr<IUnknown> ires, UINT DstSubresource, const void * pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    ComPtr<ID3D11Resource> res;
    ires.As(&res);
    device_context->UpdateSubresource(res.Get(), DstSubresource, nullptr, pSrcData, SrcRowPitch, SrcDepthPitch);
}

void DX11Context::BeginEvent(LPCWSTR Name)
{
    perf->BeginEvent(Name);
}

void DX11Context::EndEvent()
{
    perf->EndEvent();
}
