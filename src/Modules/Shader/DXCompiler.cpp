#include "Shader/DXCompiler.h"

class FileWrap
{
public:
    FileWrap(const std::string& path)
        : m_file_path(GetExecutableDir() + "/" + path)
    {
    }

    bool IsExist() const
    {
        return std::ifstream(m_file_path, std::ios::binary).good();
    }

    std::vector<uint8_t> ReadFile() const
    {
        std::ifstream file(m_file_path, std::ios::binary);

        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> res(file_size);
        file.read((char*)res.data(), file_size);
        return res;
    }

private:
    std::string m_file_path;
};

ComPtr<ID3DBlob> DXCompile(const ShaderBase& shader)
{
    std::string shader_name = shader.shader_path.substr(shader.shader_path.find_last_of("\\/") + 1);
    shader_name = shader_name.substr(0, shader_name.find(".hlsl")) + ".cso";
    FileWrap precompiled_blob(shader_name);
    if (precompiled_blob.IsExist() && shader.define.empty())
    {
        auto data = precompiled_blob.ReadFile();
        ComPtr<ID3DBlob> shader_buffer;
        D3DCreateBlob(data.size(), &shader_buffer);
        shader_buffer->GetBufferPointer();
        memcpy(shader_buffer->GetBufferPointer(), data.data(), data.size());
        return shader_buffer;
    }

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
