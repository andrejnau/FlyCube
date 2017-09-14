#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <D3Dcompiler.h>

#include <map>
#include <string>
#include <vector>

#include <Utility.h>
#include <Geometry/Geometry.h>
#include <Utilities/FileUtility.h>

enum class ShaderType
{
    kVertex,
    kPixel
};

class UniformProxy
{
public:
    UniformProxy(char* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    template<typename T>
    void operator=(const T& value)
    {
        assert(sizeof(T) == m_size);
        *reinterpret_cast<T*>(m_data) = value;
    }

private:
    char* m_data;
    size_t m_size;
};

class CBuffer
{
public:
    CBuffer(ComPtr<ID3D11Device>& device, ShaderType shader_type, ComPtr<ID3D11ShaderReflection>& reflector, UINT index)
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

    UniformProxy Uniform(const std::string& name)
    {
        if (m_vdesc.count(name))
        {
            auto& desc = m_vdesc[name];
            return UniformProxy(m_data.data() + desc.offset, desc.size);
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

class DXShader
{
public:
    DXShader(ComPtr<ID3D11Device>& device, const std::string& vertex, const std::string& pixel)
    {
        m_vertex_shader_buffer = CompileShader(device, vertex, "main", "vs_5_0");
        ASSERT_SUCCEEDED(device->CreateVertexShader(m_vertex_shader_buffer->GetBufferPointer(), m_vertex_shader_buffer->GetBufferSize(), nullptr, &vertex_shader));

        m_pixel_shader_buffer = CompileShader(device, pixel, "main", "ps_5_0");
        ASSERT_SUCCEEDED(device->CreatePixelShader(m_pixel_shader_buffer->GetBufferPointer(), m_pixel_shader_buffer->GetBufferSize(), nullptr, &pixel_shader));

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX11Mesh::Vertex, texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        ASSERT_SUCCEEDED(device->CreateInputLayout(layout, ARRAYSIZE(layout), m_vertex_shader_buffer->GetBufferPointer(),
            m_vertex_shader_buffer->GetBufferSize(), &input_layout));

        ParseVariable(device, ShaderType::kVertex, m_vertex_shader_buffer, m_vs_buffers);
        ParseVariable(device, ShaderType::kPixel, m_pixel_shader_buffer, m_ps_buffers);
    }

    CBuffer& GetVSCBuffer(const std::string& name)
    {
        auto it = m_vs_buffers.find(name);
        if (it != m_vs_buffers.end())
            return it->second;
        throw "name not found";
    }

    CBuffer& GetPSCBuffer(const std::string& name)
    {
        auto it = m_ps_buffers.find(name);
        if (it != m_ps_buffers.end())
            return it->second;
        throw "name not found";
    }

    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ComPtr<ID3D11InputLayout> input_layout;

private:
    ComPtr<ID3DBlob> CompileShader(ComPtr<ID3D11Device>& device, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
    {
        ComPtr<ID3DBlob> errors;
        ComPtr<ID3DBlob> shader_buffer;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPathW(shader_path).c_str(),
            nullptr,
            nullptr,
            entrypoint.c_str(),
            target.c_str(),
            0,
            0,
            &shader_buffer,
            &errors));
        return shader_buffer;
    }

    void ParseVariable(ComPtr<ID3D11Device>& device, ShaderType shader_type, ComPtr<ID3DBlob>& shader_buffer, std::map<std::string, CBuffer>& buffers)
    {
        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(shader_buffer->GetBufferPointer(), shader_buffer->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D11_SHADER_DESC desc = {};
        reflector->GetDesc(&desc);

        for (UINT i = 0; i < desc.ConstantBuffers; ++i)
        {
            ID3D11ShaderReflectionConstantBuffer* buffer = nullptr;
            buffer = reflector->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC bdesc = {};
            buffer->GetDesc(&bdesc);
            buffers.emplace(bdesc.Name, CBuffer(device, shader_type, reflector, i));
        }
    }

    ComPtr<ID3DBlob> m_vertex_shader_buffer;
    ComPtr<ID3DBlob> m_pixel_shader_buffer;

    std::map<std::string, CBuffer> m_vs_buffers;
    std::map<std::string, CBuffer> m_ps_buffers;
};
