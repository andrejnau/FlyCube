#pragma once

#include "Program/ShaderType.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Context/Context.h>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>
#include <cstddef>
#include <map>
#include <string>

using namespace Microsoft::WRL;

class BufferLayout
{
    Context & m_context;

public:
    BufferLayout(
        Context& context,
        const std::string& name,
        size_t buffer_size, 
        UINT slot,
        std::vector<size_t>&& data_size,
        std::vector<size_t>&& data_offset,
        std::vector<size_t>&& buf_offset
    )
        : m_context(context)
        , buffer(CreateBuffer(buffer_size))
        , slot(slot)
        , data(buffer_size)
        , data_size(std::move(data_size))
        , data_offset(std::move(data_offset))
        , buf_offset(std::move(buf_offset))
    {
        buffer->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
    }

    ComPtr<ID3D11Buffer> buffer;
    UINT slot;

    void UpdateCBuffer(const char* src_data)
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
            m_context.device_context->UpdateSubresource(buffer.Get(), 0, nullptr, data.data(), 0, 0);
    }

    template<ShaderType>
    void BindCBuffer();

    template<>
    void BindCBuffer<ShaderType::kPixel>()
    {
        m_context.device_context->PSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

    template<>
    void BindCBuffer<ShaderType::kVertex>()
    {
        m_context.device_context->VSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

    template<>
    void BindCBuffer<ShaderType::kGeometry>()
    {
        m_context.device_context->GSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

    template<>
    void BindCBuffer<ShaderType::kCompute>()
    {
        m_context.device_context->CSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
    }

private:
    std::vector<char> data;
    std::vector<size_t> data_size;
    std::vector<size_t> data_offset;
    std::vector<size_t> buf_offset;

    ComPtr<ID3D11Buffer> CreateBuffer(UINT buffer_size)
    {
        ComPtr<ID3D11Buffer> buffer;
        D3D11_BUFFER_DESC cbbd;
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = buffer_size;
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        ASSERT_SUCCEEDED(m_context.device->CreateBuffer(&cbbd, nullptr, &buffer));
        return buffer;
    }
};

class ShaderBase
{
protected:
    
public:
    virtual ~ShaderBase() = default;

    virtual void UseShader() = 0;
    virtual void BindCBuffers() = 0;
    virtual void UpdateCBuffers() = 0;
    virtual void UpdateShader() = 0;
    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) = 0;

    std::map<std::string, std::string> define;

    ShaderBase(Context& context, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : m_context(context)
        , m_shader_path(shader_path)
        , m_entrypoint(entrypoint)
        , m_target(target)
    {
    }

protected:
    void CompileShader()
    {
        std::vector<D3D_SHADER_MACRO> macro;
        for (const auto & x : define)
        {
            macro.push_back({ x.first.c_str(), x.second.c_str() });
        }
        macro.push_back({ nullptr, nullptr });

        ComPtr<ID3DBlob> errors;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPathW(m_shader_path).c_str(),
            macro.data(),
            nullptr,
            m_entrypoint.c_str(),
            m_target.c_str(),
            0,
            0,
            &m_shader_buffer,
            &errors));
    }

    Context& m_context;
    std::string m_shader_path;
    std::string m_entrypoint;
    std::string m_target;
    ComPtr<ID3DBlob> m_shader_buffer;
};

class SRVBinding
{
public:
    SRVBinding(ShaderBase& shader, uint32_t slot)
        : m_shader(shader)
        , m_slot(slot)
    {
    }

    void Attach(ComPtr<ID3D11ShaderResourceView>& srv = ComPtr<ID3D11ShaderResourceView>{})
    {
        m_shader.Attach(m_slot, srv);
    }
private:
    ShaderBase& m_shader;
    uint32_t m_slot;
};

template<ShaderType> class Shader : public ShaderBase {};

template<>
class Shader<ShaderType::kPixel> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kPixel;

    ComPtr<ID3D11PixelShader> shader;

    Shader(Context& context, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(context, shader_path, entrypoint, target)
    {
    }

    virtual void UseShader() override
    {
        m_context.device_context->PSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void UpdateShader() override
    {
        CompileShader();
        ASSERT_SUCCEEDED(m_context.device->CreatePixelShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
    }
};

template<>
class Shader<ShaderType::kVertex> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kVertex;

    ComPtr<ID3D11VertexShader> shader;
    ComPtr<ID3D11InputLayout> input_layout;

    Shader(Context& context, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(context, shader_path, entrypoint, target)
    {
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
        m_context.device_context->VSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void UpdateShader() override
    {
        CompileShader();
        ASSERT_SUCCEEDED(m_context.device->CreateVertexShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
        CreateInputLayout();
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->VSSetShaderResources(slot, 1, srv.GetAddressOf());
    }
};

template<>
class Shader<ShaderType::kGeometry> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kGeometry;

    ComPtr<ID3D11GeometryShader> shader;

    Shader(Context& context, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(context, shader_path, entrypoint, target)
    {
    }

    virtual void UseShader() override
    {
        m_context.device_context->GSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void UpdateShader() override
    {
        CompileShader();
        ASSERT_SUCCEEDED(m_context.device->CreateGeometryShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->GSSetShaderResources(slot, 1, srv.GetAddressOf());
    }
};

template<>
class Shader<ShaderType::kCompute> : public ShaderBase
{
public:
    static const ShaderType type = ShaderType::kCompute;

    ComPtr<ID3D11ComputeShader> shader;

    Shader(Context& context, const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : ShaderBase(context, shader_path, entrypoint, target)
    {
    }

    virtual void UseShader() override
    {
        m_context.device_context->CSSetShader(shader.Get(), nullptr, 0);
    }

    virtual void UpdateShader() override
    {
        CompileShader();
        ASSERT_SUCCEEDED(m_context.device->CreateComputeShader(m_shader_buffer->GetBufferPointer(), m_shader_buffer->GetBufferSize(), nullptr, &shader));
    }

    virtual void Attach(uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv) override
    {
        m_context.device_context->CSSetShaderResources(slot, 1, srv.GetAddressOf());
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

    ShaderHolderImpl(Context& context)
        : ps(context)
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

    ShaderHolderImpl(Context& context)
        : vs(context)
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

    ShaderHolderImpl(Context& context)
        : gs(context)
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

    ShaderHolderImpl(Context& context)
        : cs(context)
    {
    }
};

template<typename T> class ShaderHolder : public ShaderHolderImpl<T::type, T> { using ShaderHolderImpl::ShaderHolderImpl; };

template<typename ... Args>
class Program : public ShaderHolder<Args>...
{
public:
    Program(Context& context)
        : ShaderHolder<Args>(context)...
        , m_shaders({ ShaderHolder<Args>::GetApi()... })
        , m_context(context)
    {
        UpdateShaders();
    }

    template<typename Setup>
    Program(Context& context, const Setup& setup)
        : ShaderHolder<Args>(context)...
        , m_shaders({ ShaderHolder<Args>::GetApi()... })
        , m_context(context)
    {
        setup(*this);
        UpdateShaders();
    }

    void UseProgram()
    {
        m_context.device_context->VSSetShader(nullptr, nullptr, 0);
        m_context.device_context->GSSetShader(nullptr, nullptr, 0);
        m_context.device_context->DSSetShader(nullptr, nullptr, 0);
        m_context.device_context->HSSetShader(nullptr, nullptr, 0);
        m_context.device_context->PSSetShader(nullptr, nullptr, 0);
        m_context.device_context->CSSetShader(nullptr, nullptr, 0);
        for (auto& shader : m_shaders)
        {
            shader.get().UseShader();
            shader.get().UpdateCBuffers();
            shader.get().BindCBuffers();
        }
    }

private:
    void UpdateShaders()
    {
        for (auto& shader : m_shaders)
        {
            shader.get().UpdateShader();
        }
    }

    std::vector<std::reference_wrapper<ShaderBase>> m_shaders;
    Context& m_context;
};
