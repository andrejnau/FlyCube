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

    ComPtr<ID3D11RasterizerState> rasterizer_state;
    D3D11_RASTERIZER_DESC shadowState = {};
    shadowState.FillMode = D3D11_FILL_SOLID;
    shadowState.CullMode = D3D11_CULL_BACK;
    shadowState.DepthBias = 4096;
    device->CreateRasterizerState(&shadowState, &rasterizer_state);
    device_context->RSSetState(rasterizer_state.Get());
}

Resource::Ptr DX11Context::GetBackBuffer()
{
    DX11Resource::Ptr res = std::make_shared<DX11Resource>();

    ComPtr<ID3D11Texture2D> back_buffer;
    ASSERT_SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));
    res->resource = back_buffer;

    return res;
}

void DX11Context::ResizeBackBuffer(int width, int height)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    ASSERT_SUCCEEDED(swap_chain->GetDesc(&desc));
    ASSERT_SUCCEEDED(swap_chain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags));
}

void DX11Context::Present(const Resource::Ptr&)
{
    ASSERT_SUCCEEDED(swap_chain->Present(0, 0));
}

void DX11Context::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    current_program->ApplyBindings();
    device_context->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void DX11Context::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
    current_program->ApplyBindings();
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

void DX11Context::SetScissorRect(LONG left, LONG top, LONG right, LONG bottom)
{
    D3D11_RECT rect = { left, top, right, bottom };
    device_context->RSSetScissorRects(1, &rect);
}

Resource::Ptr DX11Context::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    DX11Resource::Ptr res = std::make_shared<DX11Resource>();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.ArraySize = depth;
    desc.MipLevels = mip_levels;
    desc.Format = format;
    desc.Usage = D3D11_USAGE_DEFAULT;

    if (bind_flag & BindFlag::kRtv)
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    if (bind_flag & BindFlag::kDsv)
        desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    if (bind_flag & BindFlag::kSrv)
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    if (depth > 1)
        desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

    uint32_t quality = 0;
    device->CheckMultisampleQualityLevels(desc.Format, msaa_count, &quality);
    desc.SampleDesc.Count = msaa_count;
    desc.SampleDesc.Quality = quality - 1;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &texture));
    res->resource = texture;

    return res;
}

std::unique_ptr<ProgramApi> DX11Context::CreateProgram()
{
    return std::make_unique<DX11ProgramApi>(*this);
}

Resource::Ptr DX11Context::CreateBuffer(uint32_t bind_flag, UINT buffer_size, size_t stride)
{
    if (buffer_size == 0)
        return DX12Resource::Ptr();

    DX11Resource::Ptr res = std::make_shared<DX11Resource>();

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

    ComPtr<ID3D11Buffer> buffer;
    ASSERT_SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffer));
    res->resource = buffer;

    return res;
}

void DX11Context::IASetIndexBuffer(Resource::Ptr ires, UINT SizeInBytes, DXGI_FORMAT Format)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);
    ComPtr<ID3D11Buffer> buf;
    res->resource.As(&buf);
    device_context->IASetIndexBuffer(buf.Get(), Format, 0);
}

void DX11Context::IASetVertexBuffer(UINT slot, Resource::Ptr ires, UINT SizeInBytes, UINT Stride)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);
    ComPtr<ID3D11Buffer> buf;
    res->resource.As(&buf);
    UINT offset = 0;
    device_context->IASetVertexBuffers(slot, 1, buf.GetAddressOf(), &Stride, &offset);
}

void DX11Context::UpdateSubresource(const Resource::Ptr& ires, UINT DstSubresource, const void * pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);
    device_context->UpdateSubresource(res->resource.Get(), DstSubresource, nullptr, pSrcData, SrcRowPitch, SrcDepthPitch);
}

void DX11Context::BeginEvent(LPCWSTR Name)
{
    perf->BeginEvent(Name);
}

void DX11Context::EndEvent()
{
    perf->EndEvent();
}
