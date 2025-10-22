#include "HLSLCompiler/DXCLoader.h"

#include "Utilities/NotReached.h"
#include "Utilities/ScopeGuard.h"
#include "Utilities/SystemUtils.h"

#include <dxc/Support/Global.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

HRESULT Test(dxc::DxcDllSupport& dll_support, ShaderBlobType target)
{
    std::string test_shader = "[shader(\"pixel\")]void main(){}";
    CComPtr<IDxcLibrary> library;
    IFR(dll_support.CreateInstance(CLSID_DxcLibrary, &library));
    CComPtr<IDxcBlobEncoding> source;
    IFR(library->CreateBlobWithEncodingFromPinned(test_shader.data(), test_shader.size(), CP_ACP, &source));

    std::vector<LPCWSTR> args;
    if (target == ShaderBlobType::kSPIRV) {
        args.emplace_back(L"-spirv");
    }

    CComPtr<IDxcOperationResult> result;
    CComPtr<IDxcCompiler> compiler;
    IFR(dll_support.CreateInstance(CLSID_DxcCompiler, &compiler));
    IFR(compiler->Compile(source, L"main.hlsl", L"", L"lib_6_3", args.data(), args.size(), nullptr, 0, nullptr,
                          &result));

#if defined(_WIN32) // Ignore missing DXIL signing library on other platforms
    CComPtr<IDxcBlobEncoding> errors;
    result->GetErrorBuffer(&errors);
    if (errors && errors->GetBufferSize() > 0) {
        return E_FAIL;
    }
#endif

    HRESULT hr = {};
    result->GetStatus(&hr);
    return hr;
}

std::unique_ptr<dxc::DxcDllSupport> Load(const std::string& path, ShaderBlobType target)
{
    std::u8string u8path(path.begin(), path.end());
#if defined(_WIN32)
    auto dxcompiler_path = std::filesystem::path(u8path) / "dxcompiler.dll";
#elif defined(__APPLE__)
    auto dxcompiler_path = std::filesystem::path(u8path) / "libdxcompiler.dylib";
#else
    auto dxcompiler_path = std::filesystem::path(u8path) / "libdxcompiler.so";
#endif
    if (!std::filesystem::exists(dxcompiler_path)) {
        return {};
    }

#if defined(_WIN32)
    auto dxil_path = std::filesystem::path(u8path) / "dxil.dll";
    std::unique_ptr<dxc::DxcDllSupport> dll_support_dxil;
    if (target == ShaderBlobType::kDXIL) {
        dll_support_dxil = std::make_unique<dxc::DxcDllSupport>();
        if (std::filesystem::exists(dxil_path)) {
            dll_support_dxil->InitializeForDll(dxil_path.string().c_str(), "DxcCreateInstance");
        }

        if (!dll_support_dxil->IsEnabled()) {
            return {};
        }
    }
#endif

    auto dll_support = std::make_unique<dxc::DxcDllSupport>();
    if (FAILED(dll_support->InitializeForDll(dxcompiler_path.string().c_str(), "DxcCreateInstance"))) {
        return {};
    }
    if (FAILED(Test(*dll_support, target))) {
        return {};
    }

    return dll_support;
}

std::unique_ptr<dxc::DxcDllSupport> GetDxcSupportImpl(ShaderBlobType target)
{
    std::vector<std::string> localions = {
        GetExecutableDir(),
        DXC_CUSTOM_LOCATION,
    };
    for (const auto& path : localions) {
        std::unique_ptr<dxc::DxcDllSupport> dll_support = Load(path, target);
        if (dll_support) {
            return dll_support;
        }
    }
    NOTREACHED();
}

} // namespace

dxc::DxcDllSupport& GetDxcSupport(ShaderBlobType target)
{
    static std::map<ShaderBlobType, std::unique_ptr<dxc::DxcDllSupport>> cache;
    auto it = cache.find(target);
    if (it == cache.end()) {
        it = cache.emplace(target, GetDxcSupportImpl(target)).first;
    }
    return *it->second;
}
