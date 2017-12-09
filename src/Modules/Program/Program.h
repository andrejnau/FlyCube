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

struct BufferLayout
{
    BufferLayout(
        ComPtr<ID3D11Device>& device,
        size_t buffer_size, 
        UINT slot,
        std::vector<size_t>&& data_size,
        std::vector<size_t>&& data_offset,
        std::vector<size_t>&& buf_offset
    )
        : buffer(CreateBuffer(device, buffer_size))
        , slot(slot)
        , data(buffer_size)
        , data_size(std::move(data_size))
        , data_offset(std::move(data_offset))
        , buf_offset(std::move(buf_offset))
    {
    }

    ComPtr<ID3D11Buffer> buffer;
    UINT slot;

    void UpdateCBuffer(ComPtr<ID3D11DeviceContext>& device_context, const char* src_data)
    {
        bool dirty = false;
        for (size_t i = 0; i < data_size.size(); ++i)
        {
            const char* ptr_src = src_data + data_offset[i];
            char* ptr_dst = data.data() + buf_offset[i];
            if (std::memcmp(ptr_dst, ptr_src, data_size[i]) != 0)
            {
                std::memcpy(ptr_dst, ptr_src, data_size[i]);
                dirty = true;
            }
        }

        if (dirty)
            device_context->UpdateSubresource(buffer.Get(), 0, nullptr, data.data(), 0, 0);
    }

    template<ShaderType>
    void BindCBuffer(ComPtr<ID3D11DeviceContext>& device_context);

    template<>
    void BindCBuffer<ShaderType::kPixel>(ComPtr<ID3D11DeviceContext>& device_context)
    {
        device_context->PSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

    template<>
    void BindCBuffer<ShaderType::kVertex>(ComPtr<ID3D11DeviceContext>& device_context)
    {
        device_context->VSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

    template<>
    void BindCBuffer<ShaderType::kGeometry>(ComPtr<ID3D11DeviceContext>& device_context)
    {
        device_context->GSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

    template<>
    void BindCBuffer<ShaderType::kCompute>(ComPtr<ID3D11DeviceContext>& device_context)
    {
        device_context->CSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

private:
    std::vector<char> data;
    std::vector<size_t> data_size;
    std::vector<size_t> data_offset;
    std::vector<size_t> buf_offset;

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
};

class ShaderBase
{
protected:
    
public:
    virtual ~ShaderBase() = default;

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) = 0;
    virtual void BindCBuffers(ComPtr<ID3D11DeviceContext>& device_context) = 0;
    virtual void UpdateCBuffers(ComPtr<ID3D11DeviceContext>& device_context) = 0;
    virtual void ApplyDefine(ComPtr<ID3D11Device>& device, const D3D_SHADER_MACRO* define) = 0;

    ShaderBase(const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : m_shader_path(shader_path)
        , m_entrypoint(entrypoint)
        , m_target(target)
    {
        CompileShader();
    }

    void CompileShader(const D3D_SHADER_MACRO* define = nullptr)
    {
        ComPtr<ID3DBlob> errors;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPathW(m_shader_path).c_str(),
            define,
            nullptr,
            m_entrypoint.c_str(),
            m_target.c_str(),
            0,
            0,
            &m_shader_buffer,
            &errors));
    }

protected:
    std::string m_shader_path;
    std::string m_entrypoint;
    std::string m_target;
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

    virtual void ApplyDefine(ComPtr<ID3D11Device>& device, const D3D_SHADER_MACRO* define) override
    {
        CompileShader(define);
        ASSERT_SUCCEEDED(device->CreatePixelShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
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
        CreateInputLayout(device);
    }

    void CreateInputLayout(ComPtr<ID3D11Device>& device)
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

        ASSERT_SUCCEEDED(device->CreateInputLayout(input_layout_desc.data(), input_layout_desc.size(), m_shader_buffer->GetBufferPointer(),
            m_shader_buffer->GetBufferSize(), &input_layout));
    }

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) override
    {
        device_context->VSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void ApplyDefine(ComPtr<ID3D11Device>& device, const D3D_SHADER_MACRO* define) override
    {
        CompileShader(define);
        ASSERT_SUCCEEDED(device->CreateVertexShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
        CreateInputLayout(device);
    }
};

template<>
class Shader<ShaderType::kGeometry> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kGeometry;

    ComPtr<ID3D11GeometryShader> shader;

    Shader(ComPtr<ID3D11Device>& device, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(shader_path, entrypoint, target)
    {
        ASSERT_SUCCEEDED(device->CreateGeometryShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) override
    {
        device_context->GSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void ApplyDefine(ComPtr<ID3D11Device>& device, const D3D_SHADER_MACRO* define) override
    {
        CompileShader(define);
        ASSERT_SUCCEEDED(device->CreateGeometryShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }
};

template<>
class Shader<ShaderType::kCompute> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kCompute;

    ComPtr<ID3D11ComputeShader> shader;

    Shader(ComPtr<ID3D11Device>& device, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(shader_path, entrypoint, target)
    {
        ASSERT_SUCCEEDED(device->CreateComputeShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }

    virtual void UseShader(ComPtr<ID3D11DeviceContext>& device_context) override
    {
        device_context->CSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void ApplyDefine(ComPtr<ID3D11Device>& device, const D3D_SHADER_MACRO* define) override
    {
        CompileShader(define);
        ASSERT_SUCCEEDED(device->CreateComputeShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
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

template<typename T>
class ShaderHolderImpl<ShaderType::kGeometry, T>
{
public:
    struct Api : public T
    {
        using T::T;
    } gs;

    ShaderBase& GetApi()
    {
        return gs;
    }

    ShaderHolderImpl(ComPtr<ID3D11Device>& device)
        : gs(device)
    {
    }
};

template<typename T>
class ShaderHolderImpl<ShaderType::kCompute, T>
{
public:
    struct Api : public T
    {
        using T::T;
    } cs;

    ShaderBase& GetApi()
    {
        return cs;
    }

    ShaderHolderImpl(ComPtr<ID3D11Device>& device)
        : cs(device)
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

    void ApplyDefine(ComPtr<ID3D11Device>& device, const D3D_SHADER_MACRO* define)
    {
        for (auto& shader : m_shaders)
        {
            shader.get().ApplyDefine(device, define);
        }
    }

private:
    std::vector<std::reference_wrapper<ShaderBase>> m_shaders;
};
