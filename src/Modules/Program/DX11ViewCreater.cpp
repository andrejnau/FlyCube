#include "DX11ViewCreater.h"
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>
#include <memory>

DX11ViewCreater::DX11ViewCreater(DX11Context& context, const IShaderBlobProvider& shader_provider)
    : m_context(context)
    , m_shader_provider(shader_provider)
    , m_program_id(shader_provider.GetProgramId())
{
}

DX11View::Ptr DX11ViewCreater::GetView(const BindKey& bind_key, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires)
{
    for (auto& type : m_shader_provider.GetShaderTypes())
    {
        m_blob_map.emplace(type, m_shader_provider.GetBlobByType(type));
    }

    if (!ires)
        return std::make_shared<DX11View>();

    DX11Resource& res = static_cast<DX11Resource&>(*ires);

    View::Ptr& iview = ires->GetView(bind_key, view_desc);
    if (iview)
        return std::static_pointer_cast<DX11View>(iview);

    DX11View::Ptr view = std::make_shared<DX11View>();
    iview = view;

    switch (bind_key.res_type)
    {
    case ResourceType::kSrv:
        view->srv = CreateSrv(bind_key.shader_type, name, bind_key.slot, view_desc, ires);
        break;
    case ResourceType::kUav:
        view->uav = CreateUAV(bind_key.shader_type, name, bind_key.slot, view_desc, ires);
        break;
    case ResourceType::kRtv:
        view->rtv = CreateRtv(bind_key.slot, view_desc, ires);
        break;
    case ResourceType::kDsv:
        view->dsv = CreateDsv(view_desc, ires);
        break;
    }

    return std::static_pointer_cast<DX11View>(view);
}

ComPtr<ID3D11ShaderResourceView> DX11ViewCreater::CreateSrv(ShaderType type, const std::string & name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr & ires)
{
    ComPtr<ID3D11ShaderResourceView> srv;

    if (!ires)
        return srv;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

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
        srv_desc.Texture2D.MostDetailedMip = view_desc.level;
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
        srv_desc.Format = tex_dec.Format;
        if (srv_desc.Format == DXGI_FORMAT_R32_TYPELESS)
        {
            srv_desc.Format = FloatFromTypeless(srv_desc.Format);
        }
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.TextureCube.MipLevels = tex_dec.MipLevels;
        srv_desc.TextureCube.MostDetailedMip = view_desc.level;
        ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(tex.Get(), &srv_desc, &srv));
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC desc = {};
        tex->GetDesc(&desc);
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srv_desc.Format = desc.Format;
        srv_desc.Texture2DArray.MostDetailedMip = view_desc.level;
        srv_desc.Texture2DArray.MipLevels = desc.MipLevels - srv_desc.Texture2D.MostDetailedMip;
        srv_desc.Texture2DArray.ArraySize = desc.ArraySize;
        switch (res_desc.ReturnType)
        {
        case D3D_RETURN_TYPE_FLOAT:
            srv_desc.Format = FloatFromTypeless(srv_desc.Format);
            break;
        }
        m_context.device->CreateShaderResourceView(tex.Get(), &srv_desc, &srv);
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC desc = {};
        tex->GetDesc(&desc);
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = desc.Format;
        if (srv_desc.Format == DXGI_FORMAT_R32_TYPELESS)
        {
            srv_desc.Format = FloatFromTypeless(srv_desc.Format);
        }
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        srv_desc.TextureCubeArray.MipLevels = desc.MipLevels;
        srv_desc.TextureCubeArray.NumCubes = desc.ArraySize / 6;
        m_context.device->CreateShaderResourceView(tex.Get(), &srv_desc, &srv);
        break;
    }
    default:
        assert(false);
        break;
    }

    return srv;
}

ComPtr<ID3D11UnorderedAccessView> DX11ViewCreater::CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    ComPtr<ID3D11UnorderedAccessView> uav;
    if (!ires)
        return uav;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

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
        uav_desc.Texture2D.MipSlice = view_desc.level;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        m_context.device->CreateUnorderedAccessView(tex.Get(), &uav_desc, &uav);
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
    {
        ComPtr<ID3D11Texture2D> tex;
        res->resource.As(&tex);
        D3D11_TEXTURE2D_DESC desc = {};
        tex->GetDesc(&desc);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = desc.Format;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        uav_desc.Texture2DArray.MipSlice = view_desc.level;
        uav_desc.Texture2DArray.ArraySize = desc.ArraySize;
        m_context.device->CreateUnorderedAccessView(tex.Get(), &uav_desc, &uav);
        break;
    }
    default:
        assert(false);
        break;
    }

    return uav;
}

ComPtr<ID3D11DepthStencilView> DX11ViewCreater::CreateDsv(const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    ComPtr<ID3D11DepthStencilView> dsv;
    if (!ires)
        return dsv;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

    ComPtr<ID3D11Texture2D> tex;
    res->resource.As(&tex);

    D3D11_TEXTURE2D_DESC desc = {};
    tex->GetDesc(&desc);

    DXGI_FORMAT format = DepthStencilFromTypeless(desc.Format);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = format;
    if (desc.ArraySize > 1)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.ArraySize = desc.ArraySize;
        dsv_desc.Texture2DArray.MipSlice = view_desc.level;
    }
    else
    {
        if (desc.SampleDesc.Count > 1)
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        else
        {
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Texture2D.MipSlice = view_desc.level;
        }
    }

    ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(tex.Get(), &dsv_desc, &dsv));

    return dsv;
}

ComPtr<ID3D11RenderTargetView> DX11ViewCreater::CreateRtv(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);

    ComPtr<ID3D11RenderTargetView> rtv;
    if (!ires)
        return rtv;

    ComPtr<ID3D11Texture2D> tex;
    res->resource.As(&tex);
    D3D11_TEXTURE2D_DESC desc = {};
    tex->GetDesc(&desc);

    DXGI_FORMAT format = desc.Format;

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = format;

    if (desc.ArraySize > 1)
    {
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtv_desc.Texture2DArray.ArraySize = desc.ArraySize;
        rtv_desc.Texture2DArray.MipSlice = view_desc.level;
    }
    else
    {
        if (desc.SampleDesc.Count > 1)
        {
            rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice = view_desc.level;
        }
    }

    ASSERT_SUCCEEDED(m_context.device->CreateRenderTargetView(res->resource.Get(), &rtv_desc, &rtv));

    return rtv;
}
