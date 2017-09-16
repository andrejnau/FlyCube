#pragma once

#include "IShaderBuffer.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <D3Dcompiler.h>

#include <map>
#include <string>
#include <vector>

#include <Utilities/DXUtility.h>
#include <Geometry/Geometry.h>
#include <Utilities/FileUtility.h>

#include "ShaderType.h"

using namespace Microsoft::WRL;

class DX11ShaderBuffer : public IShaderBuffer
{
public:
    DX11ShaderBuffer(ComPtr<ID3D11Device>& device, ShaderType shader_type, ComPtr<ID3D11ShaderReflection>& reflector, UINT index)
        : m_slot(index)
        , m_shader_type(shader_type)
    {
        ID3D11ShaderReflectionConstantBuffer* cbuffer = nullptr;
        cbuffer = reflector->GetConstantBufferByIndex(index);

        D3D11_SHADER_BUFFER_DESC bdesc = {};
        cbuffer->GetDesc(&bdesc);

        m_buffer = CreateBuffer(device, bdesc.Size);
        m_data.resize(bdesc.Size);

        for (UINT i = 0; i < bdesc.Variables; ++i)
        {
            ID3D11ShaderReflectionVariable* variable = nullptr;
            variable = cbuffer->GetVariableByIndex(i);

            D3D11_SHADER_VARIABLE_DESC orig_vdesc;
            variable->GetDesc(&orig_vdesc);

            VDesc& vdesc = m_vdesc[orig_vdesc.Name];
            vdesc.size = orig_vdesc.Size;
            vdesc.offset = orig_vdesc.StartOffset;
        }
    }

    virtual BufferProxy Uniform(const std::string& name) override
    {
        if (m_vdesc.count(name))
        {
            auto& desc = m_vdesc[name];
            return BufferProxy(m_data.data() + desc.offset, desc.size);
        }

        throw "name not found";
    }

    void Update(ComPtr<ID3D11DeviceContext>& device_context)
    {
        device_context->UpdateSubresource(m_buffer.Get(), 0, nullptr, m_data.data(), 0, 0);
    }

    void SetOnPipeline(ComPtr<ID3D11DeviceContext>& device_context)
    {
        if (m_shader_type == ShaderType::kVertex)
            device_context->VSSetConstantBuffers(m_slot, 1, m_buffer.GetAddressOf());
        else
            device_context->PSSetConstantBuffers(m_slot, 1, m_buffer.GetAddressOf());
    }

private:
    ComPtr<ID3D11Buffer> CreateBuffer(ComPtr<ID3D11Device>& device, UINT buffer_size)
    {
        ComPtr<ID3D11Buffer> buffer;
        D3D11_BUFFER_DESC cbbd;
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = buffer_size;
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        ASSERT_SUCCEEDED(device->CreateBuffer(&cbbd, nullptr, &buffer));
        return buffer;
    }

    struct VDesc
    {
        size_t size;
        size_t offset;
    };

    std::map<std::string, VDesc> m_vdesc;
    UINT m_slot;
    ShaderType m_shader_type;
    ComPtr<ID3D11Buffer> m_buffer;
    std::vector<char> m_data;
};
