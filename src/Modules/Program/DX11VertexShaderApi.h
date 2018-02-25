#pragma once

#include <Program/DX11ShaderApi.h>
#include <Context/DX11Context.h>

class DX11VertexShaderApi : public DX11ShaderApi
{
public:
    static const ShaderType type = ShaderType::kVertex;

    ComPtr<ID3D11VertexShader> shader;
    ComPtr<ID3D11InputLayout> input_layout;

    DX11VertexShaderApi(DX11Context& context)
        : DX11ShaderApi(context)
    {
    }

    virtual void BindCBuffer(uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        ComPtr<ID3D11Buffer> buf;
        res.As(&buf);
        m_context.device_context->VSSetConstantBuffers(slot, 1, buf.GetAddressOf());
    }

    void CreateInputLayout()
    {
        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), IID_PPV_ARGS(&reflector));

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

        ASSERT_SUCCEEDED(m_context.device->CreateInputLayout(input_layout_desc.data(), input_layout_desc.size(), m_shader_buffer->GetBufferPointer(),
            m_shader_buffer->GetBufferSize(), &input_layout));
    }

    virtual void UseShader() override
    {
        m_context.device_context->IASetInputLayout(input_layout.Get());
        m_context.device_context->VSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void CreateShader(const ComPtr<ID3DBlob>& blob) override
    {
        m_shader_buffer = blob;
        ASSERT_SUCCEEDED(m_context.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader));
        CreateInputLayout();
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->VSSetShaderResources(slot, 1, srv.GetAddressOf());
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav) override
    {

    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11SamplerState>& sampler) override
    {
        m_context.device_context->VSSetSamplers(slot, 1, sampler.GetAddressOf());
    }
};
