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

#include "DX11ShaderBuffer.h"

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

    DX11ShaderBuffer& GetVSCBuffer(const std::string& name)
    {
        auto it = m_vs_buffers.find(name);
        if (it != m_vs_buffers.end())
            return it->second;
        throw "name not found";
    }

    DX11ShaderBuffer& GetPSCBuffer(const std::string& name)
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

    void ParseVariable(ComPtr<ID3D11Device>& device, ShaderType shader_type, ComPtr<ID3DBlob>& shader_buffer, std::map<std::string, DX11ShaderBuffer>& buffers)
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
            buffers.emplace(bdesc.Name, DX11ShaderBuffer(device, shader_type, reflector, i));
        }
    }

    ComPtr<ID3DBlob> m_vertex_shader_buffer;
    ComPtr<ID3DBlob> m_pixel_shader_buffer;

    std::map<std::string, DX11ShaderBuffer> m_vs_buffers;
    std::map<std::string, DX11ShaderBuffer> m_ps_buffers;
};
