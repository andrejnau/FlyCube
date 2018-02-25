#pragma once

#include <Program/DX11ShaderApi.h>
#include <Context/DX11Context.h>

class DX11PixelShaderApi : public DX11ShaderApi
{
public:
    static const ShaderType type = ShaderType::kPixel;

    ComPtr<ID3D11PixelShader> shader;

    DX11PixelShaderApi(DX11Context& context)
        : DX11ShaderApi(context)
    {
    }

    virtual void BindCBuffer(uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        ComPtr<ID3D11Buffer> buf;
        res.As(&buf);
        m_context.device_context->PSSetConstantBuffers(slot, 1, buf.GetAddressOf());
    }

    virtual void UseShader() override
    {
        m_context.device_context->PSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void CreateShader(const ComPtr<ID3DBlob>& blob) override
    {
        m_shader_buffer = blob;
        ASSERT_SUCCEEDED(m_context.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader));
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav) override
    {
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11SamplerState>& sampler) override
    {
        m_context.device_context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
    }
};
