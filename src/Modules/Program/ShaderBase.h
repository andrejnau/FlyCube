#pragma once

#include <Program/ShaderType.h>
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <Utilities/State.h>
#include <map>
#include <vector>

#include <d3dcompiler.h>
#include <wrl.h>
#include <assert.h>

using namespace Microsoft::WRL;

class Context;

class ShaderBase
{
protected:
    
public:
    virtual ~ShaderBase() = default;
    virtual void UpdateShader() = 0;
    virtual void UseShader() = 0;

    ShaderBase(const std::string& shader_path, const std::string& entrypoint, const std::string& target)
        : m_shader_path(shader_path)
        , m_entrypoint(entrypoint)
        , m_target(target)
    {
    }

    std::map<std::string, std::string> define;

protected:
    ComPtr<ID3DBlob> CompileShader() const
    {
        decltype(&::D3DCompileFromFile) _D3DCompileFromFile = &::D3DCompileFromFile;
        auto& state = CurState<bool>::Instance().state;
        if (state["DXIL"])
            _D3DCompileFromFile = (decltype(&::D3DCompileFromFile))GetProcAddress(LoadLibraryA("d3dcompiler_dxc_bridge.dll"), "D3DCompileFromFile");

        ComPtr<ID3DBlob> shader_buffer;
        std::vector<D3D_SHADER_MACRO> macro;
        for (const auto & x : define)
        {
            macro.push_back({ x.first.c_str(), x.second.c_str() });
        }
        macro.push_back({ nullptr, nullptr });

        ComPtr<ID3DBlob> errors;
        ASSERT_SUCCEEDED(_D3DCompileFromFile(
            GetAssetFullPathW(m_shader_path).c_str(),
            macro.data(),
            nullptr,
            m_entrypoint.c_str(),
            m_target.c_str(),
            D3DCOMPILE_DEBUG,
            0,
            &shader_buffer,
            &errors));
        return shader_buffer;
    }

    std::string m_shader_path;
    std::string m_entrypoint;
    std::string m_target;
    ComPtr<ID3DBlob> m_shader_buffer;
};
