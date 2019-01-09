#pragma once

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
#include <Shader/ShaderBase.h>
#include <Program/ProgramApi.h>
#include "Program/BufferLayout.h"

using namespace Microsoft::WRL;

template<typename T>
class Binding
{
public:
    Binding(ProgramApi& program_api, ShaderType shader_type, ResourceType res_type, uint32_t slot, const std::string& name)
        : m_program_api(program_api)
        , m_key{ program_api.GetProgramId(), shader_type, res_type, slot }
    {
        m_program_api.SetBindingName(m_key, name);
    }

    T& Attach(const Resource::Ptr& ires = {}, const ViewDesc& view_desc = {})
    {
        m_program_api.Attach(m_key, view_desc, ires);
        return static_cast<T&>(*this);
    }

protected:
    ProgramApi& m_program_api;
    BindKey m_key;
};

class SRVBinding : public Binding<SRVBinding>
{
public:    
    SRVBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(program_api, shader_type, ResourceType::kSrv, slot, name)
    {
    }
};

class UAVBinding : public Binding<UAVBinding>
{
public:
    UAVBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(program_api, shader_type, ResourceType::kUav, slot, name)
    {
    }
};

class SamplerBinding : public Binding<SamplerBinding>
{
public:
    SamplerBinding(ProgramApi& program_api, ShaderType shader_type, const std::string& name, uint32_t slot)
        : Binding(program_api, shader_type, ResourceType::kSampler, slot, name)
    {
    }
};

class RTVBinding : public Binding<RTVBinding>
{
public:
    RTVBinding(ProgramApi& program_api, uint32_t slot)
        : Binding(program_api, ShaderType::kPixel, ResourceType::kRtv, slot, "")
    {
    }

    void Clear(const FLOAT ColorRGBA[4])
    {
        m_program_api.ClearRenderTarget(m_key.slot, ColorRGBA);
    }
};

class DSVBinding : public Binding<DSVBinding>
{
public:
    DSVBinding(ProgramApi& program_api)
        : Binding(program_api, ShaderType::kPixel, ResourceType::kDsv, 0, "")
    {
    }

    void Clear(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
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

    void UseProgram()
    {
        m_program_base->UseProgram();
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

private:
    void UpdateShaders()
    {
        EnumerateShader<Args...>([&](ShaderBase& shader)
        {
            shader.UpdateShader();
        });
        m_program_base->LinkProgram();
    }

    std::unique_ptr<ProgramApi> m_program_base;
};
