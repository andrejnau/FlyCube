#pragma once
#include <Context/Context.h>
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <Context/Context.h>
#include "ProgramApi/BufferLayout.h"
#include "ProgramApi/ProgramApi.h"

template<typename T>
class Binding
{
public:
    Binding(Context& context, ShaderType shader_type, ViewType view_type, uint32_t slot, const std::string& name)
        : m_context(context)
        , m_key{ shader_type, view_type, slot, name }
    {
    }

    T& Attach(const std::shared_ptr<Resource>& resource = {}, const LazyViewDesc& view_desc = {})
    {
        m_context.Attach(m_key, resource, view_desc);
        return static_cast<T&>(*this);
    }

protected:
    Context& m_context;
    NamedBindKey m_key;
};

class SRVBinding : public Binding<SRVBinding>
{
public:
    SRVBinding(Context& context, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(context, shader_type, ViewType::kShaderResource, slot, name)
    {
    }
};

class UAVBinding : public Binding<UAVBinding>
{
public:
    UAVBinding(Context& context, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(context, shader_type, ViewType::kUnorderedAccess, slot, name)
    {
    }
};

class SamplerBinding : public Binding<SamplerBinding>
{
public:
    SamplerBinding(Context& context, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(context, shader_type, ViewType::kSampler, slot, name)
    {
    }
};

class RTVBinding : public Binding<RTVBinding>
{
public:
    RTVBinding(Context& context, uint32_t slot)
        : Binding(context, ShaderType::kPixel, ViewType::kRenderTarget, slot, "")
    {
    }

    void Clear(const std::array<float, 4>& color)
    {
        m_context.ClearRenderTarget(m_key.slot, color);
    }
};

class DSVBinding : public Binding<DSVBinding>
{
public:
    DSVBinding(Context& context)
        : Binding(context, ShaderType::kPixel, ViewType::kDepthStencil, 0, "")
    {
    }

    void Clear(uint32_t ClearFlags, float Depth, uint8_t Stencil)
    {
        m_context.ClearDepthStencil(ClearFlags, Depth, Stencil);
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

    ShaderHolderImpl(ProgramApi& program)
        : ps(program)
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

    ShaderHolderImpl(ProgramApi& program)
        : vs(program)
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

    ShaderHolderImpl(ProgramApi& program)
        : gs(program)
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

    ShaderHolderImpl(ProgramApi& program)
        : cs(program)
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

    ShaderHolderImpl(ProgramApi& program)
        : lib(program)
    {
    }
};

template<ShaderType T, ShaderType... Ts>
constexpr bool contains()
{
    return ((T == Ts) || ...);
}

template<typename T> class ShaderHolder : public ShaderHolderImpl<T::type, T> { using ShaderHolderImpl<T::type, T>::ShaderHolderImpl; };

template<typename ... Args>
class ProgramHolder : public ShaderHolder<Args>...
{
    ProgramHolder(std::shared_ptr<ProgramApi> program_api)
        : ShaderHolder<Args>(*program_api)...
        , m_program_api(program_api)
        , m_context(program_api->GetContext())
    {
    }
public:
    ProgramHolder(Context& context)
        : ProgramHolder(context.CreateProgramApi())
    {
        UpdateShaders();
    }

    template<typename Setup>
    ProgramHolder(Context& context, const Setup& setup)
        : ProgramHolder(context.CreateProgramApi())
    {
        setup(*this);
        UpdateShaders();
    }

    template <typename T>
    bool ApplyCallback()
    {
        if constexpr (contains<ShaderType::kGeometry, Args::type...>() && T::type == ShaderType::kVertex)
        {
            static_cast<ShaderHolder<T>&>(*this).GetApi().desc.define["__INTERNAL_DO_NOT_INVERT_Y__"] = "1";
        }
        static_cast<ShaderHolder<T>&>(*this).GetApi().UpdateShader();
        return true;
    }

    template<typename ... ShadowsArgs> void DevNull(ShadowsArgs ... args) {}

    template<typename... ShadowsArgs>
    void EnumerateShader()
    {
        DevNull(ApplyCallback<ShadowsArgs>()...);
    }

    void SetRasterizeState(const RasterizerDesc& desc)
    {
        m_context.SetRasterizeState(desc);
    }

    void SetBlendState(const BlendDesc& desc)
    {
        m_context.SetBlendState(desc);
    }

    void SetDepthStencilState(const DepthStencilDesc& desc)
    {
        m_context.SetDepthStencilState(desc);
    }

    void LinkProgram()
    {
        m_program_api->GetProgram() = m_context.CreateProgram({ static_cast<ShaderHolder<Args>&>(*this).GetApi().shader ... });
    }

    operator ProgramApi& ()
    {
        return *m_program_api;
    }

private:
    void UpdateShaders()
    {
        EnumerateShader<Args...>();
        m_program_api->GetProgram() = m_context.CreateProgram({ static_cast<ShaderHolder<Args>&>(*this).GetApi().shader ... });
    }

    std::shared_ptr<ProgramApi> m_program_api;
    Context& m_context;
};
