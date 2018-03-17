#pragma once

#include "Program/ShaderType.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <Context/Context.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <Program/ShaderBase.h>
#include <Program/ProgramApi.h>
#include "Program/BufferLayout.h"

using namespace Microsoft::WRL;

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

    void Attach(const Resource::Ptr& ires = {})
    {
        m_program_api.AttachSRV(m_shader_type, m_name, m_slot, ires);
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

    void Attach(const Resource::Ptr& ires = {})
    {
        m_program_api.AttachUAV(m_shader_type, m_name, m_slot, ires);
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

class RTVBinding
{
public:
    RTVBinding(ProgramApi& program_api, uint32_t slot)
        : m_program_api(program_api)
        , m_slot(slot)
    {
    }

    RTVBinding& Attach(const Resource::Ptr& ires = {})
    {
        m_program_api.AttachRTV(m_slot, ires);
        return *this;
    }

    void Clear(const FLOAT ColorRGBA[4])
    {
        m_program_api.ClearRenderTarget(m_slot, ColorRGBA);
    }

private:
    ProgramApi& m_program_api;
    uint32_t m_slot;
};

class DSVBinding
{
public:
    DSVBinding(ProgramApi& program_api)
        : m_program_api(program_api)
    {
    }

    DSVBinding& Attach(const Resource::Ptr& ires = {})
    {
        m_program_api.AttachDSV(ires);
        return *this;
    }

    void Clear(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
    {
        m_program_api.ClearDepthStencil(ClearFlags, Depth, Stencil);
    }

private:
    ProgramApi& m_program_api;
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

    void SetMaxEvents(size_t count)
    {
        m_program_base->SetMaxEvents(count);
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

    void UseProgram()
    {
        m_program_base->UseProgram();
        EnumerateShader<Args...>([&](ShaderBase& shader)
        {
            shader.UseShader();
        });
    }

    void SetRasterizeState(const RasterizerDesc& desc)
    {
        m_program_base->SetRasterizeState(desc);
    }

    void SetBlendState(const BlendDesc& desc)
    {
        m_program_base->SetBlendState(desc);
    }

    void SetDepthStencilState(const DepthStencilDesc& desc)
    {
        m_program_base->SetDepthStencilState(desc);
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
