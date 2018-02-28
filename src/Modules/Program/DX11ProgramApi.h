#pragma once

#include <Context/DX11Context.h>
#include "Program/ProgramApi.h"

class DX11ProgramApi : public ProgramApi
{
public:
    DX11ProgramApi(DX11Context& context)
        : m_context(context)
    {
    }

    virtual void UseProgram(size_t) override
    {
        m_context.device_context->VSSetShader(vshader.Get(), nullptr, 0);
        m_context.device_context->GSSetShader(gshader.Get(), nullptr, 0);
        m_context.device_context->DSSetShader(nullptr, nullptr, 0);
        m_context.device_context->HSSetShader(nullptr, nullptr, 0);
        m_context.device_context->PSSetShader(pshader.Get(), nullptr, 0);
        m_context.device_context->CSSetShader(cshader.Get(), nullptr, 0);
        m_context.device_context->IASetInputLayout(input_layout.Get());
    }

    virtual void UpdateData(ComPtr<IUnknown> ires, const void* ptr) override
    {
        ComPtr<ID3D11Resource> buffer;
        ires.As(&buffer);
        m_context.device_context->UpdateSubresource(buffer.Get(), 0, nullptr, ptr, 0, 0);
    }

    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;
    ComPtr<ID3D11InputLayout> input_layout;
    ComPtr<ID3D11VertexShader> vshader;
    ComPtr<ID3D11PixelShader> pshader;
    ComPtr<ID3D11ComputeShader> cshader;
    ComPtr<ID3D11GeometryShader> gshader;

    void CreateInputLayout()
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

    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) override
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

    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        Attach(type, slot, CreateSrv(type, name, slot, res));
    }

    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        Attach(type, slot, CreateUAV(type, name, slot, res));
    }

    void Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv)
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

    void Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav)
    {
        switch (type)
        {
        case ShaderType::kCompute:
            m_context.device_context->CSSetUnorderedAccessViews(slot, 1, uav.GetAddressOf(), nullptr);
            break;
        }
    }

    void Attach(ShaderType type, uint32_t slot, ComPtr<ID3D11SamplerState>& sampler)
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

    virtual void AttachRTV(uint32_t slot, const ComPtr<IUnknown>& res) override
    {
    }

    virtual void AttachDSV(const ComPtr<IUnknown>& res) override
    {
    }

    virtual void UpdateOmSet() override
    {
    }

    virtual void AttachSampler(ShaderType type, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        ComPtr<ID3D11SamplerState> sampler;
        res.As(&sampler);
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

    virtual void AttachCBuffer(ShaderType type, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        ComPtr<ID3D11Buffer> buf;
        res.As(&buf);

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

    ComPtr<ID3D11ShaderResourceView> CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires)
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        if (!ires)
            return srv;

        ComPtr<ID3D11Resource> res;
        ires.As(&res);

        if (!res)
            return srv;

        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D11_SHADER_INPUT_BIND_DESC res_desc = {};
        ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

        D3D11_RESOURCE_DIMENSION dim = {};
        res->GetType(&dim);

        switch (res_desc.Dimension)
        {
        case D3D_SRV_DIMENSION_BUFFER:
        {
            ComPtr<ID3D11Buffer> buf;
            res.As(&buf);
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
            res.As(&tex);
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
            res.As(&tex);
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
            res.As(&tex);
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

    ComPtr<ID3D11UnorderedAccessView> CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires)
    {
        ComPtr<ID3D11UnorderedAccessView> uav;
        if (!ires)
            return uav;

        ComPtr<ID3D11Resource> res;
        ires.As(&res);

        if (!res)
            return uav;

        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D11_SHADER_INPUT_BIND_DESC res_desc = {};
        ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

        D3D11_RESOURCE_DIMENSION dim = {};
        res->GetType(&dim);

        switch (res_desc.Dimension)
        {
        case D3D_SRV_DIMENSION_BUFFER:
        {
            ComPtr<ID3D11Buffer> buf;
            res.As(&buf);
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

    virtual Context& GetContext() override
    {
        return m_context;
    }
private:
    DX11Context& m_context;
};
