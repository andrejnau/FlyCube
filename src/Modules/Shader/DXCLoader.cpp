#include "Shader/DXCLoader.h"
#include <Utilities/FileUtility.h>
#include <Utilities/ScopeGuard.h>
#include <vector>

DXCLoader::DXCLoader()
{
    std::wstring custom_path = utf8_to_wstring(DXC_CUSTOM_BIN);
    if (Load(custom_path))
        return;

#ifdef _WIN64
    std::wstring sdk_path = utf8_to_wstring(SDKBIN) + L"/x64";
#else
    std::wstring sdk_path = utf8_to_wstring(SDKBIN) + L"/x86";
#endif

    Load(sdk_path);
}

bool DXCLoader::Load(const std::wstring& path)
{
    std::vector<wchar_t> prev_dll_dir(GetDllDirectoryW(0, nullptr));
    GetDllDirectoryW(static_cast<DWORD>(prev_dll_dir.size()), prev_dll_dir.data());
    ScopeGuard guard = [&]
    {
        SetDllDirectoryW(prev_dll_dir.data());
    };

    SetDllDirectoryW(path.c_str());
    std::wstring dxcompiler = path + L"/dxcompiler.dll";
    HRESULT hr = m_dll_support.InitializeForDll(dxcompiler.c_str(), "DxcCreateInstance");
    if (FAILED(hr))
        return false;
    m_dll_support.CreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &library);
    m_dll_support.CreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), &reflection);
    m_dll_support.CreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), &compiler);
    return true;
}

DXCLoader::~DXCLoader()
{
    m_dll_support.Detach();
}
