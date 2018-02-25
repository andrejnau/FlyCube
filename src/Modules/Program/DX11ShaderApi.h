#pragma once

#include <Program/ShaderType.h>
#include <Program/ShaderApi.h>
#include <Context/DX11Context.h>

class DX11ShaderApi : public ShaderApi
{
public:
    virtual void UpdateData(ComPtr<IUnknown> ires, const void* ptr)
    {
        ComPtr<ID3D11Resource> buffer;
        ires.As(&buffer);
        m_context.device_context->UpdateSubresource(buffer.Get(), 0, nullptr, ptr, 0, 0);
    }

    virtual Context& GetContext() override
    {
        return m_context;
    }

    virtual void AttachSRV(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        Attach(slot, CreateSrv(name, slot, res));
    }

    virtual ComPtr<ID3D11ShaderResourceView> CreateSrv(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires) override
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        if (!ires)
            return srv;

        ComPtr<ID3D11Resource> res;
        ires.As(&res);

        if (!res)
            return srv;

        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), IID_PPV_ARGS(&reflector));

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


    virtual void AttachUAV(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        Attach(slot, CreateUAV(name, slot, res));
    }

    virtual ComPtr<ID3D11UnorderedAccessView> CreateUAV(const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires) override
    {
        ComPtr<ID3D11UnorderedAccessView> uav;
        if (!ires)
            return uav;

        ComPtr<ID3D11Resource> res;
        ires.As(&res);

        if (!res)
            return uav;

        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), IID_PPV_ARGS(&reflector));

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

    std::map<std::string, std::string> define;

    DX11ShaderApi(DX11Context& context)
        : m_context(context)
    {
    }

    DX11Context& m_context;
};
