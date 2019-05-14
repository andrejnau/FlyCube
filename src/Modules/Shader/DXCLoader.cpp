#include "Shader/DXCLoader.h"
#include <Utilities/FileUtility.h>
#include <Utilities/ScopeGuard.h>
#include <vector>

DXCLoader::DXCLoader()
{
#ifdef _WIN64
    std::wstring sdk_path = utf8_to_wstring(SDKBIN) + L"/x64";
#else
    std::wstring sdk_path = utf8_to_wstring(SDKBIN) + L"/x86";
#endif

    std::vector<wchar_t> prev_dll_dir(GetDllDirectoryW(0, nullptr));
    GetDllDirectoryW(static_cast<DWORD>(prev_dll_dir.size()), prev_dll_dir.data());
    ScopeGuard guard = [&]
    {
        SetDllDirectoryW(prev_dll_dir.data());
    };
    SetDllDirectoryW(sdk_path.c_str());

    std::wstring dxcompiler = sdk_path + L"/dxcompiler.dll";
    m_dll_support.InitializeForDll(dxcompiler.c_str(), "DxcCreateInstance");
    m_dll_support.CreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &library);
    m_dll_support.CreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), &reflection);
    m_dll_support.CreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), &compiler);
}

DXCLoader::~DXCLoader()
{
    m_dll_support.Detach();
}
