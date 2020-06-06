#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <Context/Context.h>
#include <Context/BufferLayout.h>

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

    T& GetApi()
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

    T& GetApi()
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

    T& GetApi()
    {
        return cs;
    }

    ShaderHolderImpl(Context& context)
        : cs(context)
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

    ShaderHolderImpl(Context& context)
        : lib(context)
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
public:
    ProgramHolder(Context& context)
        : ShaderHolder<Args>(context)...
        , m_context(context)
    {
        CompileShaders();
    }

    template<typename Setup>
    ProgramHolder(Context& context, const Setup& setup)
        : ShaderHolder<Args>(context)...
        , m_context(context)
    {
        setup(*this);
        CompileShaders();
    }

    operator std::shared_ptr<Program>& ()
    {
        return m_program;
    }

    void UpdateProgram()
    {
        CompileShaders();
    }

private:
    template <typename T>
    bool ApplyCallback()
    {
        auto& api = static_cast<ShaderHolder<T>&>(*this).GetApi();
        if constexpr (contains<ShaderType::kGeometry, Args::type...>() && T::type == ShaderType::kVertex)
        {
            api.desc.define["__INTERNAL_DO_NOT_INVERT_Y__"] = "1";
        }
        api.CompileShader(m_context);
        return true;
    }

    template<typename ... ShadowsArgs> void DevNull(ShadowsArgs ... args) {}

    template<typename... ShadowsArgs>
    void EnumerateShader()
    {
        DevNull(ApplyCallback<ShadowsArgs>()...);
    }

    void CompileShaders()
    {
        EnumerateShader<Args...>();
        m_program = m_context.CreateProgram({ static_cast<ShaderHolder<Args>&>(*this).GetApi().shader ... });
    }

    Context& m_context;
    std::shared_ptr<Program> m_program;
};
