#pragma once

inline void CreateRtvSrv(Context& context, uint32_t msaa_count, int width, int height, ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv)
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    uint32_t quality = 0;
    context.device->CheckMultisampleQualityLevels(texture_desc.Format, msaa_count, &quality);
    texture_desc.SampleDesc.Count = msaa_count;
    texture_desc.SampleDesc.Quality = quality - 1;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(context.device->CreateTexture2D(&texture_desc, nullptr, &texture));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.Texture2D.MipLevels = 1;

    if (msaa_count == 1)
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    }
    else
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }

    ASSERT_SUCCEEDED(context.device->CreateShaderResourceView(texture.Get(), &srv_desc, &srv));
    ASSERT_SUCCEEDED(context.device->CreateRenderTargetView(texture.Get(), nullptr, &rtv));
}

inline void CreateDsv(Context& context, uint32_t msaa_count, int width, int height, ComPtr<ID3D11DepthStencilView>& depth_stencil_view)
{
    D3D11_TEXTURE2D_DESC depth_stencil_desc = {};
    depth_stencil_desc.Width = width;
    depth_stencil_desc.Height = height;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    uint32_t quality = 0;
    context.device->CheckMultisampleQualityLevels(depth_stencil_desc.Format, msaa_count, &quality);
    depth_stencil_desc.SampleDesc.Count = msaa_count;
    depth_stencil_desc.SampleDesc.Quality = quality - 1;

    ComPtr<ID3D11Texture2D> depth_stencil_buffer;
    ASSERT_SUCCEEDED(context.device->CreateTexture2D(&depth_stencil_desc, nullptr, &depth_stencil_buffer));
    ASSERT_SUCCEEDED(context.device->CreateDepthStencilView(depth_stencil_buffer.Get(), nullptr, &depth_stencil_view));
}

template<typename T>
inline ComPtr<ID3D11ShaderResourceView> CreateBufferSRV(Context& context, const std::vector<T>& v)
{
    ComPtr<ID3D11ShaderResourceView> srv;
    if (v.empty())
        return srv;
    
    ComPtr<ID3D11Buffer> buffer;
    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.ByteWidth = v.size() * sizeof(T);
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = sizeof(T);
    ASSERT_SUCCEEDED(context.device->CreateBuffer(&buffer_desc, nullptr, &buffer));
    
    context.device_context->UpdateSubresource(buffer.Get(), 0, nullptr, v.data(), 0, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC  srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = v.size();
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    ASSERT_SUCCEEDED(context.device->CreateShaderResourceView(buffer.Get(), &srv_desc, &srv));

    return srv;
}

inline void CreateShadowDSV(Context& context, const Settings& settings, ComPtr<ID3D11DepthStencilView>& depth_stencil_view, ComPtr<ID3D11ShaderResourceView>& srv)
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = settings.s_size;
    texture_desc.Height = settings.s_size;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 6;
    texture_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    dsv_desc.Texture2DArray.ArraySize = texture_desc.ArraySize;

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MipLevels = 1;

    ComPtr<ID3D11Texture2D> cube_texture;
    ASSERT_SUCCEEDED(context.device->CreateTexture2D(&texture_desc, nullptr, &cube_texture));
    ASSERT_SUCCEEDED(context.device->CreateDepthStencilView(cube_texture.Get(), &dsv_desc, &depth_stencil_view));
    ASSERT_SUCCEEDED(context.device->CreateShaderResourceView(cube_texture.Get(), &srv_desc, &srv));
}
