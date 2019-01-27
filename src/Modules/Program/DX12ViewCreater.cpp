#include "DX12ViewCreater.h"
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>
#include <memory>
#include "DX12ViewDescCreater.h"

DX12ViewCreater::DX12ViewCreater(DX12Context& context, const IShaderBlobProvider& shader_provider)
    : m_context(context)
    , m_shader_provider(shader_provider)
{
    if (CurState::Instance().DXIL)
        _D3DReflect = (decltype(&::D3DReflect))GetProcAddress(LoadLibraryA("d3dcompiler_dxc_bridge.dll"), "D3DReflect");
}

DX12View::Ptr DX12ViewCreater::GetEmptyDescriptor(ResourceType res_type)
{
    static std::map<ResourceType, View::Ptr> empty_handles;
    auto it = empty_handles.find(res_type);
    if (it == empty_handles.end())
        it = empty_handles.emplace(res_type, m_context.m_view_pool.AllocateDescriptor(res_type)).first;
    return std::static_pointer_cast<DX12View>(it->second);
}

DX12View::Ptr DX12ViewCreater::GetView(const BindKey& bind_key, const ViewDesc& view_desc, const std::string& name, const Resource::Ptr& ires)
{
    if (!ires)
        return GetEmptyDescriptor(bind_key.res_type);

    DX12Resource& res = static_cast<DX12Resource&>(*ires);

    View::Ptr& view = ires->GetView(bind_key, view_desc);
    if (view)
        return std::static_pointer_cast<DX12View>(view);

    view = m_context.m_view_pool.AllocateDescriptor(bind_key.res_type);

    DX12View& handle = static_cast<DX12View&>(*view);

    switch (bind_key.res_type)
    {
    case ResourceType::kSrv:
        CreateSrv(bind_key.shader_type, name, bind_key.slot, view_desc, res, handle);
        break;
    case ResourceType::kUav:
        CreateUAV(bind_key.shader_type, name, bind_key.slot, view_desc, res, handle);
        break;
    case ResourceType::kCbv:
        CreateCBV(bind_key.shader_type, bind_key.slot, res, handle);
        break;
    case ResourceType::kSampler:
        CreateSampler(bind_key.shader_type, bind_key.slot, res, handle);
        break;
    case ResourceType::kRtv:
        CreateRTV(bind_key.slot, view_desc, res, handle);
        break;
    case ResourceType::kDsv:
        CreateDSV(view_desc, res, handle);
        break;
    }

    return std::static_pointer_cast<DX12View>(view);
}

void DX12ViewCreater::CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle)
{
    ComPtr<ID3D12ShaderReflection> reflector;
    auto shader_blob = m_shader_provider.GetBlobByType(type);
    _D3DReflect(shader_blob.data, shader_blob.size, IID_PPV_ARGS(&reflector));
    D3D12_SHADER_INPUT_BIND_DESC binding_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &binding_desc));
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = DX12GeSRVDesc(binding_desc, view_desc, res);
    m_context.device->CreateShaderResourceView(res.default_res.Get(), &srv_desc, handle.GetCpuHandle());
}

void DX12ViewCreater::CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle)
{
    ComPtr<ID3D12ShaderReflection> reflector;
    auto shader_blob = m_shader_provider.GetBlobByType(type);
    _D3DReflect(shader_blob.data, shader_blob.size, IID_PPV_ARGS(&reflector));
    D3D12_SHADER_INPUT_BIND_DESC binding_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &binding_desc));
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = DX12GetUAVDesc(binding_desc, view_desc, res);
    m_context.device->CreateUnorderedAccessView(res.default_res.Get(), nullptr, &uav_desc, handle.GetCpuHandle());
}

void DX12ViewCreater::CreateRTV(uint32_t slot, const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle)
{
    ComPtr<ID3D12ShaderReflection> reflector;
    auto shader_blob = m_shader_provider.GetBlobByType(ShaderType::kPixel);
    _D3DReflect(shader_blob.data, shader_blob.size, IID_PPV_ARGS(&reflector));
    D3D12_SIGNATURE_PARAMETER_DESC binding_desc = {};
    reflector->GetOutputParameterDesc(slot, &binding_desc);
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = DX12GetRTVDesc(binding_desc, view_desc, res);
    m_context.device->CreateRenderTargetView(res.default_res.Get(), &rtv_desc, handle.GetCpuHandle());
}

void DX12ViewCreater::CreateDSV(const ViewDesc& view_desc, const DX12Resource& res, DX12View& handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = DX12GetDSVDesc(view_desc, res);
    m_context.device->CreateDepthStencilView(res.default_res.Get(), &dsv_desc, handle.GetCpuHandle());
}

void DX12ViewCreater::CreateCBV(ShaderType type, uint32_t slot, const DX12Resource& res, DX12View& handle)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = res.default_res->GetGPUVirtualAddress();
    desc.SizeInBytes = (res.default_res->GetDesc().Width + 255) & ~255;
    m_context.device->CreateConstantBufferView(&desc, handle.GetCpuHandle());
}

void DX12ViewCreater::CreateSampler(ShaderType type, uint32_t slot, const DX12Resource& res, DX12View& handle)
{
    m_context.device->CreateSampler(&res.sampler_desc, handle.GetCpuHandle());
}
