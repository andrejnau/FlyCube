#include "DX11ProgramApi.h"

DX11ProgramApi::DX11ProgramApi(DX11Context & context)
    : m_context(context)
{
}

void DX11ProgramApi::SetMaxEvents(size_t) {}

void DX11ProgramApi::UseProgram()
{
    m_context.UseProgram(*this);
    m_context.device_context->VSSetShader(vshader.Get(), nullptr, 0);
    m_context.device_context->GSSetShader(gshader.Get(), nullptr, 0);
    m_context.device_context->DSSetShader(nullptr, nullptr, 0);
    m_context.device_context->HSSetShader(nullptr, nullptr, 0);
    m_context.device_context->PSSetShader(pshader.Get(), nullptr, 0);
    m_context.device_context->CSSetShader(cshader.Get(), nullptr, 0);

    ID3D11ShaderResourceView* empty_srv[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    m_context.device_context->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);

    ID3D11UnorderedAccessView* empty_uav[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    m_context.device_context->CSSetUnorderedAccessViews(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, empty_uav, nullptr);

    ID3D11RenderTargetView* empty_rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* empty_dsv = nullptr;
    m_context.device_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, empty_rtv, empty_dsv);

    m_context.device_context->IASetInputLayout(input_layout.Get());
}

void DX11ProgramApi::AttachCBuffer(ShaderType type, UINT slot, BufferLayout & buffer_layout)
{
    m_cbv_buffer.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(m_context.CreateBuffer(BindFlag::kCbv, buffer_layout.GetBuffer().size(), 0)));
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(buffer_layout));
}

void DX11ProgramApi::ApplyBindings()
{
    for (auto &x : m_cbv_layout)
    {
        BufferLayout& buffer = x.second;
        auto& res = m_cbv_buffer[x.first];
        if (buffer.SyncData())
        {
            m_context.UpdateSubresource(res, 0, buffer.GetBuffer().data(), 0, 0);
        }

        AttachCBV(std::get<0>(x.first), std::get<1>(x.first), res);
    }

    std::vector<ID3D11RenderTargetView*> rtv_ptr;
    for (auto & rtv : m_rtvs)
    {
        rtv_ptr.emplace_back(rtv.Get());
    }
    ComPtr<ID3D11DepthStencilView> dsv = m_dsv.Get();
    m_context.device_context->OMSetRenderTargets(rtv_ptr.size(), rtv_ptr.data(), dsv.Get());
}

void DX11ProgramApi::CreateInputLayout()
{
    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[ShaderType::kVertex]->GetBufferPointer(), m_blob_map[ShaderType::kVertex]->GetBufferSize(), IID_PPV_ARGS(&reflector));

    D3D11_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc;
    for (UINT i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);

        D3D11_INPUT_ELEMENT_DESC layout = {};
        layout.SemanticName = param_desc.SemanticName;
        layout.SemanticIndex = param_desc.SemanticIndex;
        layout.InputSlot = i;
        layout.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout.InstanceDataStepRate = 0;

        if (param_desc.Mask == 1)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (param_desc.Mask <= 3)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (param_desc.Mask <= 7)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (param_desc.Mask <= 15)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        input_layout_desc.push_back(layout);
    }

    ASSERT_SUCCEEDED(m_context.device->CreateInputLayout(input_layout_desc.data(), input_layout_desc.size(), m_blob_map[ShaderType::kVertex]->GetBufferPointer(),
        m_blob_map[ShaderType::kVertex]->GetBufferSize(), &input_layout));
}

void DX11ProgramApi::OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob)
{
    m_blob_map[type] = blob;
    switch (type)
    {
    case ShaderType::kVertex:
        ASSERT_SUCCEEDED(m_context.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vshader));
        CreateInputLayout();
        break;
    case ShaderType::kPixel:
        ASSERT_SUCCEEDED(m_context.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pshader));
        break;
    case ShaderType::kCompute:
        ASSERT_SUCCEEDED(m_context.device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &cshader));
        break;
    case ShaderType::kGeometry:
        ASSERT_SUCCEEDED(m_context.device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &gshader));
        break;
    }
}

void DX11ProgramApi::AttachSRV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
    Attach(type, slot, CreateSrv(type, name, slot, res));
}

void DX11ProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
    Attach(type, slot, CreateUAV(type, name, slot, res));
}

void DX11ProgramApi::Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11SamplerState>& sampler)
{
    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    }
}

void DX11ProgramApi::AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc & desc)
{
    ComPtr<ID3D11SamplerState> sampler;

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

    ASSERT_SUCCEEDED(m_context.device->CreateSamplerState(&sampler_desc, &sampler));

    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    }
}

void DX11ProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr& ires)
{
    if (m_rtvs.size() >= slot)
        m_rtvs.resize(slot + 1);
    m_rtvs[slot] = CreateRtv(ires);
}

void DX11ProgramApi::AttachDSV(const Resource::Ptr& ires)
{
    m_dsv = CreateDsv(ires);
}

void DX11ProgramApi::ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4])
{
    m_context.device_context->ClearRenderTargetView(m_rtvs[slot].Get(), ColorRGBA);
}

void DX11ProgramApi::ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    m_context.device_context->ClearDepthStencilView(m_dsv.Get(), ClearFlags, Depth, Stencil);
}

void DX11ProgramApi::AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & ires)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);

    ComPtr<ID3D11Buffer> buf;
    res->resource.As(&buf);

    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    }
}

ComPtr<ID3D11ShaderResourceView> DX11ProgramApi::CreateSrv(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & ires)
{
    ComPtr<ID3D11ShaderResourceView> srv;

    if (!ires)
        return srv;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

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
    return srv;
}

ComPtr<ID3D11UnorderedAccessView> DX11ProgramApi::CreateUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & ires)
{
    ComPtr<ID3D11UnorderedAccessView> uav;
    if (!ires)
        return uav;

    auto res = std::static_pointer_cast<DX11Resource>(ires);

    if (!res)
        return uav;

    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

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
    default:
        assert(false);
        break;
    }
    return uav;
}

void DX11ProgramApi::Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav)
{
    switch (type)
    {
    case ShaderType::kCompute:
        m_context.device_context->CSSetUnorderedAccessViews(slot, 1, uav.GetAddressOf(), nullptr);
        break;
    }
}

void DX11ProgramApi::Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv)
{
    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    }
}
