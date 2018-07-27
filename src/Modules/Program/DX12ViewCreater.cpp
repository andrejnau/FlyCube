#include "DX12ViewCreater.h"
#include <Utilities/State.h>
#include <Texture/DXGIFormatHelper.h>

DX12ViewCreater::DX12ViewCreater(DX12Context& context, const IShaderBlobProvider& shader_provider)
    : m_context(context)
    , m_shader_provider(shader_provider)
{
    if (CurState::Instance().DXIL)
        _D3DReflect = (decltype(&::D3DReflect))GetProcAddress(LoadLibraryA("d3dcompiler_dxc_bridge.dll"), "D3DReflect");
}

void DX12ViewCreater::CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle)
{
    if (handle.IsInit())
        return;

    ComPtr<ID3D12ShaderReflection> reflector;
    auto shader_blob = m_shader_provider.GetBlobByType(type);
    _D3DReflect(shader_blob.data, shader_blob.size, IID_PPV_ARGS(&reflector));

    D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

    D3D12_RESOURCE_DESC desc = res.default_res->GetDesc();

    switch (res_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = static_cast<UINT>(desc.Width / res.stride);
        srv_desc.Buffer.StructureByteStride = res.stride;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        m_context.device->CreateShaderResourceView(res.default_res.Get(), &srv_desc, handle.GetCpuHandle());

        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2D:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = desc.MipLevels;
        srv_desc.Format = desc.Format;
        switch (res_desc.ReturnType)
        {
        case D3D_RETURN_TYPE_FLOAT:
            srv_desc.Format = FloatFromTypeless(srv_desc.Format);
            break;
        }
        m_context.device->CreateShaderResourceView(res.default_res.Get(), &srv_desc, handle.GetCpuHandle());
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = desc.Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        m_context.device->CreateShaderResourceView(res.default_res.Get(), &srv_desc, handle.GetCpuHandle());
        break;
    }
    case D3D_SRV_DIMENSION_TEXTURECUBE:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT; // TODO tex_dec.Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.TextureCube.MipLevels = desc.MipLevels;
        m_context.device->CreateShaderResourceView(res.default_res.Get(), &srv_desc, handle.GetCpuHandle());
        break;
    }
    default:
        assert(false);
        break;
    }

    handle.SetInit();
}

void DX12ViewCreater::CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle)
{
    if (handle.IsInit())
        return;

    ComPtr<ID3D12ShaderReflection> reflector;
    auto shader_blob = m_shader_provider.GetBlobByType(type);
    _D3DReflect(shader_blob.data, shader_blob.size, IID_PPV_ARGS(&reflector));

    D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
    ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

    D3D12_RESOURCE_DESC desc = res.default_res->GetDesc();

    switch (res_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = static_cast<UINT>(desc.Width / res.stride);
        uav_desc.Buffer.StructureByteStride = res.stride;
        m_context.device->CreateUnorderedAccessView(res.default_res.Get(), nullptr, &uav_desc, handle.GetCpuHandle());

        break;
    }
    default:
        assert(false);
        break;
    }

    handle.SetInit();
}

void DX12ViewCreater::CreateCBV(ShaderType type, uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle)
{
    if (handle.IsInit())
        return;

    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = res.default_res->GetGPUVirtualAddress();
    desc.SizeInBytes = (res.default_res->GetDesc().Width + 255) & ~255;

    m_context.device->CreateConstantBufferView(&desc, handle.GetCpuHandle());

    handle.SetInit();
}

void DX12ViewCreater::CreateSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc, DescriptorHeapRange& handle)
{
    D3D12_SAMPLER_DESC sampler_desc = {};

    switch (desc.filter)
    {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }

    switch (desc.mode)
    {
    case SamplerTextureAddressMode::kWrap:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    }

    switch (desc.func)
    {
    case SamplerComparisonFunc::kNever:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    }

    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    sampler_desc.MaxAnisotropy = 1;

    m_context.device->CreateSampler(&sampler_desc, handle.GetCpuHandle());

    handle.SetInit();
}

void DX12ViewCreater::CreateRTV(uint32_t slot, const DX12Resource& res, DescriptorHeapRange& handle)
{
    if (handle.IsInit())
        return;

    const D3D12_RESOURCE_DESC& desc = res.desc;
    DXGI_FORMAT format = desc.Format;

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = format;

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
        rtv_desc.Buffer.NumElements = 1;
    }
    else
    {
        if (desc.SampleDesc.Count > 1)
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        else
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    }
    m_context.device->CreateRenderTargetView(res.default_res.Get(), &rtv_desc, handle.GetCpuHandle());

    handle.SetInit();
}

void DX12ViewCreater::CreateDSV(const DX12Resource& res, DescriptorHeapRange& handle)
{
    if (handle.IsInit())
        return;

    auto desc = res.default_res->GetDesc();
    DXGI_FORMAT format = DepthStencilFromTypeless(desc.Format);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = format;
    if (desc.DepthOrArraySize > 1)
    {
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
    }
    else
    {
        if (desc.SampleDesc.Count > 1)
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        else
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    }

    m_context.device->CreateDepthStencilView(res.default_res.Get(), &dsv_desc, handle.GetCpuHandle());

    handle.SetInit();
}
