#include "DX11ViewCreater.h"
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>
#include <memory>

DX11ViewCreater::DX11ViewCreater(DX11Context& context, const IShaderBlobProvider& shader_provider)
    : m_context(context)
    , m_shader_provider(shader_provider)
    , m_program_id(shader_provider.GetProgramId())
{
    for (auto& type : m_shader_provider.GetShaderTypes())
    {
        m_blob_map.emplace(type, m_shader_provider.GetBlobByType(type));
    }
}

ComPtr<ID3D11ShaderResourceView> DX11ViewCreater::CreateSrv(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & ires)
{
    ComPtr<ID3D11ShaderResourceView> srv;

    if (!ires)
        return srv;

    auto res = std::static_pointer_cast<DX11Resource>(ires);
    BindKey key = { m_program_id, type, ResourceType::kSrv, slot };
    auto it = res->srv.find(key);
    if (it != res->srv.end())
        return it->second;

    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[type].data, m_blob_map[type].size, IID_PPV_ARGS(&reflector));

    D3D11_SHADER_INPUT_BIND_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

    D3D11_RESOURCE_DIMENSION dim = {};
    res->resource->GetType(&dim);

    switch (res_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    {
        ComPtr<ID3D11Buffer> buf;
        res->resource.As(&buf);
        D3D11_BUFFER_DESC buf_dec = {};
        buf->GetDesc(&buf_dec);

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = buf_dec.ByteWidth / buf_dec.StructureByteStride;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

        ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(buf.Get(), &srv_desc, &srv));
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2D:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC tex_dec = {};
        tex->GetDesc(&tex_dec);
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = tex_dec.Format;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = tex_dec.MipLevels;
        ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(tex.Get(), &srv_desc, &srv));
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC tex_dec = {};
        tex->GetDesc(&tex_dec);
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = tex_dec.Format;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(tex.Get(), &srv_desc, &srv));
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURECUBE:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC tex_dec = {};
        tex->GetDesc(&tex_dec);
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT; // TODO tex_dec.Format;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.TextureCube.MipLevels = tex_dec.MipLevels;
        ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(tex.Get(), &srv_desc, &srv));
        break;
    }
    default:
        assert(false);
        break;
    }

    res->srv.emplace(key, srv);

    return srv;
}

ComPtr<ID3D11UnorderedAccessView> DX11ViewCreater::CreateUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & ires)
{
    ComPtr<ID3D11UnorderedAccessView> uav;
    if (!ires)
        return uav;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

    BindKey key = { m_program_id, type, ResourceType::kUav, slot };
    auto it = res->uav.find(key);
    if (it != res->uav.end())
        return it->second;

    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[type].data, m_blob_map[type].size, IID_PPV_ARGS(&reflector));

    D3D11_SHADER_INPUT_BIND_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

    D3D11_RESOURCE_DIMENSION dim = {};
    res->resource->GetType(&dim);

    switch (res_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    {
        ComPtr<ID3D11Buffer> buf;
        res->resource.As(&buf);
        D3D11_BUFFER_DESC buf_dec = {};
        buf->GetDesc(&buf_dec);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = buf_dec.ByteWidth / buf_dec.StructureByteStride;
        m_context.device->CreateUnorderedAccessView(buf.Get(), &uav_desc, &uav);
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2D:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC tex_decs = {};
        tex->GetDesc(&tex_decs);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = tex_decs.Format;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        m_context.device->CreateUnorderedAccessView(tex.Get(), &uav_desc, &uav);
        break;
    }
    default:
        assert(false);
        break;
    }

    res->uav.emplace(key, uav);

    return uav;
}

ComPtr<ID3D11DepthStencilView> DX11ViewCreater::CreateDsv(const Resource::Ptr& ires)
{
    ComPtr<ID3D11DepthStencilView> dsv;
    if (!ires)
        return dsv;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

    BindKey key = { m_program_id, ShaderType::kPixel, ResourceType::kDsv, 0 };
    auto it = res->dsv.find(key);
    if (it != res->dsv.end())
        return it->second;

    ComPtr<ID3D11Texture2D> tex;
    res->resource.As(&tex);

    D3D11_TEXTURE2D_DESC tex_dec = {};
    tex->GetDesc(&tex_dec);

    if (tex_dec.Format == DXGI_FORMAT_R32_TYPELESS) // TODO
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.ArraySize = tex_dec.ArraySize;
        ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(tex.Get(), &dsv_desc, &dsv));
    }
    else
    {
        ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(tex.Get(), nullptr, &dsv));
    }

    res->dsv.emplace(key, dsv);

    return dsv;
}

ComPtr<ID3D11RenderTargetView> DX11ViewCreater::CreateRtv(uint32_t slot, const Resource::Ptr& ires)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);

    ComPtr<ID3D11RenderTargetView> rtv;
    if (!ires)
        return rtv;

    BindKey key = { m_program_id, ShaderType::kPixel, ResourceType::kRtv, slot };
    auto it = res->rtv.find(key);
    if (it != res->rtv.end())
        return it->second;

    ASSERT_SUCCEEDED(m_context.device->CreateRenderTargetView(res->resource.Get(), nullptr, &rtv));

    res->rtv.emplace(key, rtv);

    return rtv;
}
