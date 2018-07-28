#include "Context/DX11Context.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Utilities/DXUtility.h>
#include <Program/DX11ProgramApi.h>
#include "Context/DXGIUtility.h"

DX11Context::DX11Context(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
    DWORD create_device_flags = 0;
#if defined(_DEBUG)
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<IDXGIFactory4> dxgi_factory;
    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));
    ComPtr<IDXGIAdapter1> adapter = GetHardwareAdapter(dxgi_factory.Get());
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;

    ComPtr<ID3D11Device> tmp_device;
    ComPtr<ID3D11DeviceContext> tmp_device_context;

    ASSERT_SUCCEEDED(D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        create_device_flags,
        &feature_level,
        1,
        D3D11_SDK_VERSION,
        &tmp_device,
        nullptr,
        &tmp_device_context));
    tmp_device.As(&device);
    tmp_device_context.As(&device_context);

    m_swap_chain = CreateSwapChain(device, dxgi_factory, glfwGetWin32Window(window), width, height, FrameCount);

    device_context.As(&perf);

#if defined(_DEBUG)
    ComPtr<ID3D11InfoQueue> info_queue;
    if (SUCCEEDED(device.As(&info_queue)))
    {
        info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
    }
#endif
}

std::unique_ptr<ProgramApi> DX11Context::CreateProgram()
{
    return std::make_unique<DX11ProgramApi>(*this);
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

    /*if (!(bind_flag & BindFlag::kDsv) && (bind_flag & BindFlag::kSrv))
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_TILED;
    }*/

    uint32_t quality = 0;
    device->CheckMultisampleQualityLevels(desc.Format, msaa_count, &quality);
    desc.SampleDesc.Count = msaa_count;
    desc.SampleDesc.Quality = quality - 1;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &texture));
    res->resource = texture;

    // if ((bind_flag & BindFlag::kDsv))
    return res;

    if (!(bind_flag & BindFlag::kSrv))
        return res;

    UINT pNumTilesForEntireResource = {};
    D3D11_PACKED_MIP_DESC pPackedMipDesc = {};
    D3D11_TILE_SHAPE pStandardTileShapeForNonPackedMips = {};
    UINT pNumSubresourceTilings = mip_levels;
    std::vector<D3D11_SUBRESOURCE_TILING> pSubresourceTilingsForNonPackedMips(pNumSubresourceTilings);
    device->GetResourceTiling(texture.Get(), &pNumTilesForEntireResource, &pPackedMipDesc, &pStandardTileShapeForNonPackedMips,
        &pNumSubresourceTilings, 0, pSubresourceTilingsForNonPackedMips.data());

    ComPtr<ID3D11Buffer> tile_pool;
    D3D11_BUFFER_DESC buf_desc = {};
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = 64 * 1024 * (pNumTilesForEntireResource+10);
    buf_desc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;

    ASSERT_SUCCEEDED(device->CreateBuffer(&buf_desc, nullptr, &tile_pool));
    res->tile_pool.push_back(tile_pool);

    UINT start = 0;
    if (!(bind_flag & BindFlag::kRtv))
    {
        for (UINT i = 0; i < pPackedMipDesc.NumStandardMips; ++i)
        {
            for (int x = 0; x < pSubresourceTilingsForNonPackedMips[i].WidthInTiles; ++x)
            {
                for (int y = 0; y < pSubresourceTilingsForNonPackedMips[i].HeightInTiles; ++y)
                {
                    for (int z = 0; z < pSubresourceTilingsForNonPackedMips[i].DepthInTiles; ++z)
                    {
                        D3D11_TILED_RESOURCE_COORDINATE coord = { x, y, z, i };
                        D3D11_TILE_REGION_SIZE reg_size = {};
                        reg_size.NumTiles = 1;
                        ASSERT_SUCCEEDED(device_context->UpdateTileMappings(
                            texture.Get(),
                            1,
                            &coord,
                            &reg_size,
                            tile_pool.Get(),
                            1,
                            nullptr,
                            &start,
                            nullptr,
                            0
                        ));

                        start += reg_size.NumTiles;
                    }
                }
            }
        }

        if (pPackedMipDesc.NumPackedMips > 0)
        {
            D3D11_TILED_RESOURCE_COORDINATE coord = { 0, 0, 0, pPackedMipDesc.NumStandardMips };
            D3D11_TILE_REGION_SIZE reg_size = {};
            reg_size.NumTiles = pPackedMipDesc.NumTilesForPackedMips;

            ASSERT_SUCCEEDED(device_context->UpdateTileMappings(
                texture.Get(),
                1,
                &coord,
                &reg_size,
                tile_pool.Get(),
                1,
                nullptr,
                &start,
                nullptr,
                0
            ));

            start += reg_size.NumTiles;
        }
    }
    else
    {
        D3D11_TILED_RESOURCE_COORDINATE coord = { 0, 0, 0, 0 };
        D3D11_TILE_REGION_SIZE reg_size = {};
        reg_size.bUseBox = true;
        reg_size.Width = pSubresourceTilingsForNonPackedMips[0].WidthInTiles;
        reg_size.Height = pSubresourceTilingsForNonPackedMips[0].HeightInTiles;
        reg_size.Depth = pSubresourceTilingsForNonPackedMips[0].DepthInTiles;
        reg_size.NumTiles = reg_size.Width * reg_size.Height * reg_size.Depth;
        ASSERT_SUCCEEDED(device_context->UpdateTileMappings(
            texture.Get(),
            1,
            &coord,
            &reg_size,
            tile_pool.Get(),
            1,
            nullptr,
            &start,
            nullptr,
            0
        ));
    }
    return res;
}

Resource::Ptr DX11Context::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    if (buffer_size == 0)
        return DX11Resource::Ptr();

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

Resource::Ptr DX11Context::CreateSampler(const SamplerDesc & desc)
{
    DX11Resource::Ptr res = std::make_shared<DX11Resource>();

    D3D11_SAMPLER_DESC sampler_desc = {};

    switch (desc.filter)
    {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }

    switch (desc.mode)
    {
    case SamplerTextureAddressMode::kWrap:
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        break;
    }

    switch (desc.func)
    {
    case SamplerComparisonFunc::kNever:
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
        break;
    }

    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    sampler_desc.MaxAnisotropy = 1;

    ASSERT_SUCCEEDED(device->CreateSamplerState(&sampler_desc, &res->sampler));

    return res;
}

void DX11Context::UpdateSubresource(const Resource::Ptr& ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);
    device_context->UpdateSubresource(res->resource.Get(), DstSubresource, nullptr, pSrcData, SrcRowPitch, SrcDepthPitch);
    int b = 0;
}

void DX11Context::SetViewport(float width, float height)
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

void DX11Context::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    D3D11_RECT rect = { left, top, right, bottom };
    device_context->RSSetScissorRects(1, &rect);
}

void DX11Context::IASetIndexBuffer(Resource::Ptr ires, uint32_t SizeInBytes, DXGI_FORMAT Format)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);
    ComPtr<ID3D11Buffer> buf;
    res->resource.As(&buf);
    device_context->IASetIndexBuffer(buf.Get(), Format, 0);
}

void DX11Context::IASetVertexBuffer(uint32_t slot, Resource::Ptr ires, uint32_t SizeInBytes, uint32_t Stride)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);
    ComPtr<ID3D11Buffer> buf;
    res->resource.As(&buf);
    uint32_t offset = 0;
    device_context->IASetVertexBuffers(slot, 1, buf.GetAddressOf(), &Stride, &offset);
}

void DX11Context::BeginEvent(LPCWSTR Name)
{
    perf->BeginEvent(Name);
}

void DX11Context::EndEvent()
{
    perf->EndEvent();
}

void DX11Context::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
    m_current_program->ApplyBindings();
    device_context->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void DX11Context::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
    m_current_program->ApplyBindings();
    device_context->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

Resource::Ptr DX11Context::GetBackBuffer()
{
    DX11Resource::Ptr res = std::make_shared<DX11Resource>();

    ComPtr<ID3D11Texture2D> back_buffer;
    ASSERT_SUCCEEDED(m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));
    res->resource = back_buffer;

    return res;
}

void DX11Context::Present(const Resource::Ptr&)
{
    ASSERT_SUCCEEDED(m_swap_chain->Present(0, 0));
}

void DX11Context::UseProgram(DX11ProgramApi& program_api)
{
    m_current_program = &program_api;
}

void DX11Context::ResizeBackBuffer(int width, int height)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    ASSERT_SUCCEEDED(m_swap_chain->GetDesc(&desc));
    ASSERT_SUCCEEDED(m_swap_chain->ResizeBuffers(FrameCount, width, height, desc.BufferDesc.Format, desc.Flags));
}
