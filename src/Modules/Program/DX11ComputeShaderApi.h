#pragma once

#include <Program/DX11ShaderApi.h>
#include <Context/DX11Context.h>

class DX11ComputeShaderApi : public DX11ShaderApi
{
public:
    static const ShaderType type = ShaderType::kCompute;

    ComPtr<ID3D11ComputeShader> shader;

    DX11ComputeShaderApi(DX11Context& context)
        : DX11ShaderApi(context)
    {
    }

    virtual void BindCBuffer(uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        ComPtr<ID3D11Buffer> buf;
        res.As(&buf);
        m_context.device_context->CSSetConstantBuffers(slot, 1, buf.GetAddressOf());
    }

    virtual void UseShader() override
    {
        m_context.device_context->CSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void CreateShader(const ComPtr<ID3DBlob>& blob) override
    {
        m_shader_buffer = blob;
        ASSERT_SUCCEEDED(m_context.device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader));
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->CSSetShaderResources(slot, 1, srv.GetAddressOf());
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav) override
    {
        m_context.device_context->CSSetUnorderedAccessViews(slot, 1, uav.GetAddressOf(), nullptr);
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11SamplerState>& sampler) override
    {
        m_context.device_context->CSSetSamplers(slot, 1, sampler.GetAddressOf());
    }
};