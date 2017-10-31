#pragma once

#include "Program/ShaderType.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>
#include <cstddef>
#include <map>
#include <string>

using namespace Microsoft::WRL;

class ShaderBase
{
public:
    virtual ~ShaderBase() = default;

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) = 0;
    virtual void BindCBuffers(ComPtr<ID3D11DeviceContext>& device_context) = 0;
    virtual void UpdateCBuffers(ComPtr<ID3D11DeviceContext>& device_context) = 0;

    static ComPtr<ID3D11Buffer> CreateBuffer(ComPtr<ID3D11Device>& device, UINT buffer_size)
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

    ShaderBase(const std::string& shader_path, const std::string& entrypoint, const std::string& target)
    {
        ComPtr<ID3DBlob> errors;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPathW(shader_path).c_str(),
            nullptr,
            nullptr,
            entrypoint.c_str(),
            target.c_str(),
            0,
            0,
            &m_shader_buffer,
            &errors));
    }

protected:
    ComPtr<ID3DBlob> m_shader_buffer;
};

template<ShaderType> class Shader : public ShaderBase {};

template<>
class Shader<ShaderType::kPixel> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kPixel;

    ComPtr<ID3D11PixelShader> shader;

    Shader(ComPtr<ID3D11Device>& device, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(shader_path, entrypoint, target)
    {
        ASSERT_SUCCEEDED(device->CreatePixelShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) override
    {
        device_context->PSSetShader(shader.Get(), nullptr, 0);
    }
};

template<>
class Shader<ShaderType::kVertex> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kVertex;

    ComPtr<ID3D11VertexShader> shader;
    ComPtr<ID3D11InputLayout> input_layout;

    Shader(ComPtr<ID3D11Device>& device, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(shader_path, entrypoint, target)
    {
        ASSERT_SUCCEEDED(device->CreateVertexShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX11Mesh::Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX11Mesh::Vertex, texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        ASSERT_SUCCEEDED(device->CreateInputLayout(layout, ARRAYSIZE(layout), m_shader_buffer->GetBufferPointer(),
            m_shader_buffer->GetBufferSize(), &input_layout));
    }

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) override
    {
        device_context->VSSetShader(shader.Get(), nullptr, 0);
    }
};

template<ShaderType, typename T> class ShaderHolderImpl {};

template<typename T>
class ShaderHolderImpl<ShaderType::kPixel, T>
{
public:
    struct Api : public T
    {
        using T::T;
    } ps;

    ShaderBase& GetApi()
    {
        return ps;
    }

    ShaderHolderImpl(ComPtr<ID3D11Device>& device)
        : ps(device)
    {
    }
};

template<typename T>
class ShaderHolderImpl<ShaderType::kVertex, T>
{
public:
    struct Api : public T
    {
        using T::T;
    } vs;

    ShaderBase& GetApi()
    {
        return vs;
    }

    ShaderHolderImpl(ComPtr<ID3D11Device>& device)
        : vs(device)
    {
    }
};

template<typename T> class ShaderHolder : public ShaderHolderImpl<T::type, T> { using ShaderHolderImpl::ShaderHolderImpl; };

template<typename ... Args>
class Program : public ShaderHolder<Args>...
{
public:
    Program(ComPtr<ID3D11Device>& device)
        : ShaderHolder<Args>(device)...
        , m_shaders({ ShaderHolder<Args>::GetApi()... })
    {
    }

    void UseProgram(ComPtr<ID3D11DeviceContext>& device_context)
    {
        for (auto& shader : m_shaders)
        {
            shader.get().UseShader(device_context);
            shader.get().UpdateCBuffers(device_context);
            shader.get().BindCBuffers(device_context);
        }
    }

private:
    std::vector<std::reference_wrapper<ShaderBase>> m_shaders;
};
