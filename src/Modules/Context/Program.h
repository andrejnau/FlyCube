#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <Device/Device.h>
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

    ShaderHolderImpl(Device& device)
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

    T& GetApi()
    {
        return vs;
    }

    ShaderHolderImpl(Device& device)
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

    T& GetApi()
    {
        return gs;
    }

    ShaderHolderImpl(Device& device)
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

    T& GetApi()
    {
        return cs;
    }

    ShaderHolderImpl(Device& device)
        : cs(device)
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

    ShaderHolderImpl(Device& device)
        : lib(device)
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
    ProgramHolder(Device& device)
        : ShaderHolder<Args>(device)...
        , m_device(device)
    {
        CompileShaders();
    }

    template<typename Setup>
    ProgramHolder(Device& device, const Setup& setup)
        : ShaderHolder<Args>(device)...
        , m_device(device)
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
        api.CompileShader(m_device);
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
        m_program = m_device.CreateProgram({ static_cast<ShaderHolder<Args>&>(*this).GetApi().shader ... });
    }

    Device& m_device;
    std::shared_ptr<Program> m_program;
};
