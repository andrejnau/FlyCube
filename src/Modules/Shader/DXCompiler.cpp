#include "Shader/DXCompiler.h"

ComPtr<ID3DBlob> DXCompile(const ShaderBase& shader)
{
    decltype(&::D3DCompileFromFile) _D3DCompileFromFile = &::D3DCompileFromFile;
    if (CurState::Instance().DXIL)
        _D3DCompileFromFile = (decltype(&::D3DCompileFromFile))GetProcAddress(LoadLibraryA("d3dcompiler_dxc_bridge.dll"), "D3DCompileFromFile");

    ComPtr<ID3DBlob> shader_buffer;
    std::vector<D3D_SHADER_MACRO> macro;
    for (const auto & x : shader.define)
    {
        macro.push_back({ x.first.c_str(), x.second.c_str() });
    }
    macro.push_back({ nullptr, nullptr });

    ComPtr<ID3DBlob> errors;
    ASSERT_SUCCEEDED(_D3DCompileFromFile(
        GetAssetFullPathW(shader.shader_path).c_str(),
        macro.data(),
        nullptr,
        shader.entrypoint.c_str(),
        shader.target.c_str(),
        D3DCOMPILE_DEBUG,
        0,
        &shader_buffer,
        &errors));
    return shader_buffer;
}
