#include "Shader/DXCompiler.h"
#include "Shader/DXCLoader.h"

constexpr bool g_force_dxil = false;

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
        file.read(reinterpret_cast<char*>(res.data()), file_size);
        return res;
    }

private:
    std::string m_file_path;
};

bool GetBlobFromCache(const std::string& shader_path, ComPtr<ID3DBlob>& shader_buffer)
{
    std::string shader_name = shader_path.substr(shader_path.find_last_of("\\/") + 1);
    shader_name = shader_name.substr(0, shader_name.find(".hlsl")) + ".cso";
    FileWrap precompiled_blob(shader_name);
    if (!precompiled_blob.IsExist())
        return false;
    auto data = precompiled_blob.ReadFile();
    D3DCreateBlob(data.size(), &shader_buffer);
    shader_buffer->GetBufferPointer();
    memcpy(shader_buffer->GetBufferPointer(), data.data(), data.size());
    return true;
}

static std::string GetShaderTarget(ShaderType type, const std::string& model)
{
    switch (type)
    {
    case ShaderType::kPixel:
        return "ps_" + model;
    case ShaderType::kVertex:
        return "vs_" + model;
    case ShaderType::kGeometry:
        return "gs_" + model;
    case ShaderType::kCompute:
        return "cs_" + model;
    case ShaderType::kLibrary:
        return "lib_" + model;
    default:
        assert(false);
        return "";
    }
}

ComPtr<ID3DBlob> DXBCCompile(const ShaderDesc& shader)
{
    ComPtr<ID3DBlob> shader_buffer;
    std::vector<D3D_SHADER_MACRO> macro;
    for (const auto & x : shader.define)
    {
        macro.push_back({ x.first.c_str(), x.second.c_str() });
    }
    macro.push_back({ nullptr, nullptr });
    std::string target = GetShaderTarget(shader.type, "5_1");
    ComPtr<ID3DBlob> errors;
    ASSERT_SUCCEEDED(D3DCompileFromFile(
        GetAssetFullPathW(shader.shader_path).c_str(),
        macro.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        shader.entrypoint.c_str(),
        target.c_str(),
        D3DCOMPILE_DEBUG,
        0,
        &shader_buffer,
        &errors));
    if (errors)
        OutputDebugStringA(reinterpret_cast<char*>(errors->GetBufferPointer()));
    return shader_buffer;
}

class IncludeHandler : public IDxcIncludeHandler
{
public:
    IncludeHandler(DXCLoader& loader, const std::wstring& base_path)
        : m_loader(loader)
        , m_base_path(base_path)
    {
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) override { return E_NOTIMPL; }
    ULONG STDMETHODCALLTYPE AddRef() override { return E_NOTIMPL; }
    ULONG STDMETHODCALLTYPE Release() override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE LoadSource(
        _In_ LPCWSTR pFilename,
        _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override
    {
        std::wstring path = m_base_path + pFilename;
        ComPtr<IDxcBlobEncoding> source;
        HRESULT hr = m_loader.library->CreateBlobFromFile(
            path.c_str(),
            nullptr,
            &source);
        if (SUCCEEDED(hr) && ppIncludeSource)
            *ppIncludeSource = source.Detach();
        return hr;
    }

private:
    DXCLoader& m_loader;
    const std::wstring& m_base_path;
};

ComPtr<ID3DBlob> DXCCompile(const ShaderDesc& shader, const DXOption& option)
{
    DXCLoader loader(!option.spirv);

    std::wstring shader_path = GetAssetFullPathW(shader.shader_path);
    std::wstring shader_dir = shader_path.substr(0, shader_path.find_last_of(L"\\/") + 1);

    ComPtr<IDxcBlobEncoding> source;
    ASSERT_SUCCEEDED(loader.library->CreateBlobFromFile(
        shader_path.c_str(),
        nullptr,
        &source));

    std::wstring target = utf8_to_wstring(GetShaderTarget(shader.type, "6_3"));
    std::wstring entrypoint = utf8_to_wstring(shader.entrypoint);
    std::vector<std::pair<std::wstring, std::wstring>> defines_store;
    std::vector<DxcDefine> defines;
    for (const auto& define : shader.define)
    {
        defines_store.emplace_back(utf8_to_wstring(define.first), utf8_to_wstring(define.second));
        defines.push_back({ defines_store.back().first.c_str(), defines_store.back().second.c_str() });
    }

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"/Zi");
    if (option.spirv)
    {
        arguments.push_back(L"-spirv");
        bool vs_or_gs = target.find(L"vs") != -1 || target.find(L"gs") != -1;
        if (option.spirv_invert_y && vs_or_gs)
            arguments.push_back(L"-fvk-invert-y");
        arguments.push_back(L"-fvk-use-dx-layout");
        arguments.push_back(L"-fspv-target-env=vulkan1.1");
        arguments.push_back(L"-fspv-extension=SPV_NV_ray_tracing");
    }

    ComPtr<IDxcOperationResult> result;
    IncludeHandler include_handler(loader, shader_dir);
    ASSERT_SUCCEEDED(loader.compiler->Compile(
        source.Get(),
        L"main.hlsl",
        entrypoint.c_str(),
        target.c_str(),
        arguments.data(), static_cast<UINT32>(arguments.size()),
        defines.data(), static_cast<UINT32>(defines.size()),
        &include_handler,
        &result
    ));

    HRESULT hr = {};
    result->GetStatus(&hr);
    ComPtr<ID3DBlob> shader_buffer;
    if (SUCCEEDED(hr))
    {
        ComPtr<IDxcBlob> tmp_blob;
        ASSERT_SUCCEEDED(result->GetResult(&tmp_blob));
        tmp_blob.As(&shader_buffer);
    }
    else
    {
        ComPtr<IDxcBlobEncoding> errors;
        result->GetErrorBuffer(&errors);
        OutputDebugStringA(reinterpret_cast<char*>(errors->GetBufferPointer()));
    }
    return shader_buffer;
}

ComPtr<ID3DBlob> DXCompile(const ShaderDesc& shader, const DXOption& option)
{
    bool dxc_target = shader.type == ShaderType::kLibrary;
    bool different_options = !shader.define.empty();
    different_options |= g_force_dxil != dxc_target;
    different_options |= option.spirv;

    ComPtr<ID3DBlob> shader_buffer;
    if (!different_options && GetBlobFromCache(shader.shader_path, shader_buffer))
        return shader_buffer;

    if (dxc_target || option.spirv || g_force_dxil)
        return DXCCompile(shader, option);
    else
        return DXBCCompile(shader);
}
