#pragma once

#include "Program/ShaderType.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Context/DX11Context.h>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <Program/ShaderBase.h>
#include <Program/ProgramApi.h>

using namespace Microsoft::WRL;

class BufferLayout
{
    ProgramApi& m_shader;
public:
    BufferLayout(
        ProgramApi& shader,
        const std::string& name,
        size_t buffer_size, 
        UINT slot,
        std::vector<size_t>&& data_size,
        std::vector<size_t>&& data_offset,
        std::vector<size_t>&& buf_offset
    )
        : m_shader(shader)
        , buffer(shader.GetContext().CreateBuffer(BindFlag::kCbv, buffer_size, 0, name))
        , slot(slot)
        , data(buffer_size)
        , data_size(std::move(data_size))
        , data_offset(std::move(data_offset))
        , buf_offset(std::move(buf_offset))
    {
    }

    ComPtr<IUnknown> buffer;
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
            m_shader.UpdateData(buffer, data.data());
    }

    ComPtr<IUnknown> GetBuffer()
    {
        return buffer;
    }

private:
    std::vector<char> data;
    std::vector<size_t> data_size;
    std::vector<size_t> data_offset;
    std::vector<size_t> buf_offset;
};

class SRVBinding
{
public:    
    SRVBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : m_program_api(program_api)
        , m_shader_type(shader_type)
        , m_name(name)
        , m_slot(slot)
    {
    }

    void Attach(const ComPtr<IUnknown>& res = {})
    {
        m_program_api.AttachSRV(m_shader_type, m_name, m_slot, res);
    }

private:
    ProgramApi& m_program_api;
    ShaderType m_shader_type;
    std::string m_name;
    uint32_t m_slot;
};

class UAVBinding
{
public:
    UAVBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : m_program_api(program_api)
        , m_shader_type(shader_type)
        , m_name(name)
        , m_slot(slot)
    {
    }

    void Attach(const ComPtr<IUnknown>& res = {})
    {
        m_program_api.AttachUAV(m_shader_type, m_name, m_slot, res);
    }

private:
    ProgramApi& m_program_api;
    ShaderType m_shader_type;
    std::string m_name;
    uint32_t m_slot;
};

class SamplerBinding
{
public:
    SamplerBinding(ProgramApi& program_api, ShaderType shader_type, uint32_t slot)
        : m_program_api(program_api)
        , m_shader_type(shader_type)
        , m_slot(slot)
    {
    }

    void Attach(const SamplerDesc& desc)
    {
        m_program_api.AttachSampler(m_shader_type, m_slot, desc);
    }

private:
    ProgramApi& m_program_api;
    ShaderType m_shader_type;
    uint32_t m_slot;
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

    T& GetApi()
    {
        return ps;
    }

    ShaderHolderImpl(ProgramApi& program_base)
        : ps(program_base)
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

    T& GetApi()
    {
        return vs;
    }

    ShaderHolderImpl(ProgramApi& program_base)
        : vs(program_base)
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

    T& GetApi()
    {
        return gs;
    }

    ShaderHolderImpl(ProgramApi& program_base)
        : gs(program_base)
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

    T& GetApi()
    {
        return cs;
    }

    ShaderHolderImpl(ProgramApi& program_base)
        : cs(program_base)
    {
    }
};

template<typename T> class ShaderHolder : public ShaderHolderImpl<T::type, T> { using ShaderHolderImpl::ShaderHolderImpl; };

template<typename ... Args>
class Program : public ShaderHolder<Args>...
{
    Program(std::unique_ptr<ProgramApi>&& program_base)
        : ShaderHolder<Args>(*program_base)...
        , m_program_base(std::move(program_base))
    {
    }
public:
    Program(Context& context)
        : Program(context.CreateProgram())
    {
        UpdateShaders();
    }

    template<typename Setup>
    Program(Context& context, const Setup& setup)
        : Program(context.CreateProgram())
    {
        setup(*this);
        UpdateShaders();
    }

    using shader_callback = std::function<void(ShaderBase&)>;

    template <typename T>
    bool ApplyCallback(shader_callback fn)
    {
        fn(static_cast<ShaderHolder<T>&>(*this).GetApi());
        return true;
    }

    template<typename ... Args> void DevNull(Args ... args) {}

    template<typename... Args>
    void EnumerateShader(shader_callback fn)
    {
        DevNull(ApplyCallback<Args>(fn)...);
    }

    void UseProgram(size_t draws)
    {
        m_program_base->UseProgram(draws);
        EnumerateShader<Args...>([&](ShaderBase& shader)
        {
            shader.UpdateCBuffers();
            shader.BindCBuffers();
        });
    }

private:
    void UpdateShaders()
    {
        EnumerateShader<Args...>([&](ShaderBase& shader)
        {
            shader.UpdateShader();
        });
    }

    std::unique_ptr<ProgramApi> m_program_base;
};
