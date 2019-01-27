#include "DX11ViewCreater.h"
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>
#include <memory>
#include "DX11ViewDescCreater.h"

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

    View::Ptr& iview = ires->GetView(bind_key, view_desc);
    if (iview)
        return std::static_pointer_cast<DX11View>(iview);

    DX11View::Ptr view = std::make_shared<DX11View>();
    iview = view;

    auto res = std::static_pointer_cast<DX11Resource>(ires);
    switch (bind_key.res_type)
    {
    case ResourceType::kSrv:
        CreateSrv(bind_key.shader_type, name, bind_key.slot, view_desc, res, view->srv);
        break;
    case ResourceType::kUav:
        CreateUAV(bind_key.shader_type, name, bind_key.slot, view_desc, res, view->uav);
        break;
    case ResourceType::kRtv:
        CreateRtv(bind_key.slot, view_desc, res, view->rtv);
        break;
    case ResourceType::kDsv:
        CreateDsv(view_desc, res, view->dsv);
        break;
    }

    return view;
}

void DX11ViewCreater::CreateSrv(ShaderType type, const std::string & name, uint32_t slot, const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11ShaderResourceView>& srv)
{
    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[type].data, m_blob_map[type].size, IID_PPV_ARGS(&reflector));
    D3D11_SHADER_INPUT_BIND_DESC binding_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &binding_desc));
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = DX11GeSRVDesc(binding_desc, view_desc, res->resource);
    m_context.device->CreateShaderResourceView(res->resource.Get(), &srv_desc, &srv);
}

void DX11ViewCreater::CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11UnorderedAccessView>& uav)
{
    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[type].data, m_blob_map[type].size, IID_PPV_ARGS(&reflector));
    D3D11_SHADER_INPUT_BIND_DESC binding_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &binding_desc));
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = DX11GetUAVDesc(binding_desc, view_desc, res->resource);
    m_context.device->CreateUnorderedAccessView(res->resource.Get(), &uav_desc, &uav);
}

void DX11ViewCreater::CreateRtv(uint32_t slot, const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11RenderTargetView>& rtv)
{
    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[ShaderType::kPixel].data, m_blob_map[ShaderType::kPixel].size, IID_PPV_ARGS(&reflector));
    D3D11_SIGNATURE_PARAMETER_DESC binding_desc = {};
    reflector->GetOutputParameterDesc(slot, &binding_desc);
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = DX11GetRTVDesc(binding_desc, view_desc, res->resource);
    ASSERT_SUCCEEDED(m_context.device->CreateRenderTargetView(res->resource.Get(), &rtv_desc, &rtv));
}

void DX11ViewCreater::CreateDsv(const ViewDesc& view_desc, const DX11Resource::Ptr& res, ComPtr<ID3D11DepthStencilView>& dsv)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = DX11GetDSVDesc(view_desc, res->resource);
    ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(res->resource.Get(), &dsv_desc, &dsv));
}
