#pragma once

#include <Context/Context.h>
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <Shader/ShaderBase.h>
#include <ProgramApi/ProgramApi.h>
#include "ProgramApi/BufferLayout.h"

template<typename T>
class Binding
{
public:
    Binding(ProgramApi& program_api, ShaderType shader_type, ViewType view_type, uint32_t slot, const std::string& name)
        : m_program_api(program_api)
        , m_key{ program_api.GetProgramId(), shader_type, view_type, slot }
    {
        m_program_api.SetBindingName(m_key, name);
    }

    T& Attach(const std::shared_ptr<Resource>& resource = {}, const LazyViewDesc& view_desc = {})
    {
        m_program_api.Attach(m_key, resource, view_desc);
        return static_cast<T&>(*this);
    }

protected:
    ProgramApi& m_program_api;
    BindKeyOld m_key;
};

class SRVBinding : public Binding<SRVBinding>
{
public:    
    SRVBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(program_api, shader_type, ViewType::kShaderResource, slot, name)
    {
    }
};

class UAVBinding : public Binding<UAVBinding>
{
public:
    UAVBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(program_api, shader_type, ViewType::kUnorderedAccess, slot, name)
    {
    }
};

class SamplerBinding : public Binding<SamplerBinding>
{
public:
    SamplerBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(program_api, shader_type, ViewType::kSampler, slot, name)
    {
    }
};

class RTVBinding : public Binding<RTVBinding>
{
public:
    RTVBinding(ProgramApi& program_api, uint32_t slot)
        : Binding(program_api, ShaderType::kPixel, ViewType::kRenderTarget, slot, "")
    {
    }

    void Clear(const std::array<float, 4>& color)
    {
        m_program_api.ClearRenderTarget(m_key.slot, color);
    }
};

class DSVBinding : public Binding<DSVBinding>
{
public:
    DSVBinding(ProgramApi& program_api)
        : Binding(program_api, ShaderType::kPixel, ViewType::kDepthStencil, 0, "")
    {
    }

    void Clear(uint32_t ClearFlags, float Depth, uint8_t Stencil)
    {
        m_program_api.ClearDepthStencil(ClearFlags, Depth, Stencil);
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

template<typename T>
class ShaderHolderImpl<ShaderType::kLibrary, T>
{
public:
    struct Api : public T
    {
        using T::T;
    } lib;

    T& GetApi()
    {
        return lib;
    }

    ShaderHolderImpl(ProgramApi& program_base)
        : lib(program_base)
    {
    }
};

template<typename T> class ShaderHolder : public ShaderHolderImpl<T::type, T> { using ShaderHolderImpl<T::type, T>::ShaderHolderImpl; };

template<typename ... Args>
class ProgramHolder : public ShaderHolder<Args>...
{
    ProgramHolder(std::shared_ptr<ProgramApi>&& program_base)
        : ShaderHolder<Args>(*program_base)...
        , m_program_base(std::move(program_base))
    {
    }
public:
    ProgramHolder(Context& context)
        : ProgramHolder(context.CreateProgram())
    {
        UpdateShaders();
    }

    template<typename Setup>
    ProgramHolder(Context& context, const Setup& setup)
        : ProgramHolder(context.CreateProgram())
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

    template<typename ... ShadowsArgs> void DevNull(ShadowsArgs ... args) {}

    template<typename... ShadowsArgs>
    void EnumerateShader(shader_callback fn)
    {
        DevNull(ApplyCallback<ShadowsArgs>(fn)...);
    }

    void LinkProgram()
    {
        m_program_base->LinkProgram();
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

    operator ProgramApi&()
    {
        return *m_program_base;
    }

private:
    void UpdateShaders()
    {
        EnumerateShader<Args...>([&](ShaderBase& shader)
        {
            shader.UpdateShader();
        });
        m_program_base->LinkProgram();
    }

    std::shared_ptr<ProgramApi> m_program_base;
};
