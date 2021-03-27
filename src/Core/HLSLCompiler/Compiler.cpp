#include "HLSLCompiler/Compiler.h"
#include "HLSLCompiler/DXCLoader.h"
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <deque>
#include <iostream>
#include <vector>
#include <d3dcompiler.h>
#include <cassert>
#include <wrl.h>
using namespace Microsoft::WRL;

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
    case ShaderType::kAmplification:
        return "as_" + model;
    case ShaderType::kMesh:
        return "ms_" + model;
    case ShaderType::kLibrary:
        return "lib_" + model;
    default:
        assert(false);
        return "";
    }
}

class IncludeHandler : public IDxcIncludeHandler
{
public:
    IncludeHandler(ComPtr<IDxcLibrary> library, const std::wstring& base_path)
        : m_library(library)
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
        HRESULT hr = m_library->CreateBlobFromFile(
            path.c_str(),
            nullptr,
            &source
        );
        if (SUCCEEDED(hr) && ppIncludeSource)
            *ppIncludeSource = source.Detach();
        return hr;
    }

private:
    ComPtr<IDxcLibrary> m_library;
    const std::wstring& m_base_path;
};

std::vector<uint8_t> Compile(const ShaderDesc& shader, ShaderBlobType blob_type)
{
    decltype(auto) dxc_support = GetDxcSupport(blob_type);

    std::wstring shader_path = utf8_to_wstring(shader.shader_path);
    std::wstring shader_dir = shader_path.substr(0, shader_path.find_last_of(L"\\/") + 1);

    ComPtr<IDxcLibrary> library;
    dxc_support.CreateInstance(CLSID_DxcLibrary, library.GetAddressOf());
    ComPtr<IDxcBlobEncoding> source;
    ASSERT_SUCCEEDED(library->CreateBlobFromFile(
        shader_path.c_str(),
        nullptr,
        &source)
    );

    std::wstring target = utf8_to_wstring(GetShaderTarget(shader.type, shader.model));
    std::wstring entrypoint = utf8_to_wstring(shader.entrypoint);
    std::vector<std::pair<std::wstring, std::wstring>> defines_store;
    std::vector<DxcDefine> defines;
    for (const auto& define : shader.define)
    {
        defines_store.emplace_back(utf8_to_wstring(define.first), utf8_to_wstring(define.second));
        defines.push_back({ defines_store.back().first.c_str(), defines_store.back().second.c_str() });
    }

    std::vector<LPCWSTR> arguments;
    std::deque<std::wstring> dynamic_arguments;
    arguments.push_back(L"/Zi");
    arguments.push_back(L"/Qembed_debug");
    uint32_t space = 0;
    if (blob_type == ShaderBlobType::kSPIRV)
    {
        arguments.emplace_back(L"-spirv");
        arguments.emplace_back(L"-fspv-target-env=vulkan1.2");
        arguments.emplace_back(L"-fspv-extension=KHR");
        arguments.emplace_back(L"-fspv-extension=SPV_NV_mesh_shader");
        arguments.emplace_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
        arguments.emplace_back(L"-fspv-extension=SPV_EXT_shader_viewport_index_layer");
        arguments.emplace_back(L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1");
        arguments.emplace_back(L"-fspv-extension=SPV_GOOGLE_user_type");
        arguments.emplace_back(L"-fvk-use-dx-layout");
        arguments.emplace_back(L"-fspv-reflect");
        space = static_cast<uint32_t>(shader.type);
    }

    arguments.emplace_back(L"-auto-binding-space");
    dynamic_arguments.emplace_back(std::to_wstring(space));
    arguments.emplace_back(dynamic_arguments.back().c_str());

    ComPtr<IDxcOperationResult> result;
    IncludeHandler include_handler(library, shader_dir);
    ComPtr<IDxcCompiler> compiler;
    dxc_support.CreateInstance(CLSID_DxcCompiler, compiler.GetAddressOf());
    ASSERT_SUCCEEDED(compiler->Compile(
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
    std::vector<uint8_t> blob;
    if (SUCCEEDED(hr))
    {
        ComPtr<IDxcBlob> dxc_blob;
        ASSERT_SUCCEEDED(result->GetResult(&dxc_blob));
        blob.assign((uint8_t*)dxc_blob->GetBufferPointer(), (uint8_t*)dxc_blob->GetBufferPointer() + dxc_blob->GetBufferSize());
    }
    else
    {
        ComPtr<IDxcBlobEncoding> errors;
        result->GetErrorBuffer(&errors);
        OutputDebugStringA(reinterpret_cast<char*>(errors->GetBufferPointer()));
        std::cout << reinterpret_cast<char*>(errors->GetBufferPointer()) << std::endl;
    }
    return blob;
}
