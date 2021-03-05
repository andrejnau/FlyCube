#include "HLSLCompiler/DXCLoader.h"
#include <Utilities/FileUtility.h>
#include <Utilities/ScopeGuard.h>
#include <vector>

DXCLoader::DXCLoader(bool dxil_required)
{
    if (Load(GetExecutableDir(), dxil_required))
        return;
    if (Load(DXC_CUSTOM_LOCATION, dxil_required))
        return;
    if (Load(GetEnv("VULKAN_SDK") + "/Bin/", dxil_required))
        return;
    Load(DXC_DEFAULT_LOCATION, dxil_required);
}

bool DXCLoader::Load(const std::string& path, bool dxil_required)
{
    std::wstring wpath = utf8_to_wstring(path);
    if (dxil_required && !PathFileExistsW((wpath + L"/dxil.dll").c_str()))
        return false;

    std::vector<wchar_t> prev_dll_dir(GetDllDirectoryW(0, nullptr));
    GetDllDirectoryW(static_cast<DWORD>(prev_dll_dir.size()), prev_dll_dir.data());
    ScopeGuard guard = [&]
    {
        SetDllDirectoryW(prev_dll_dir.data());
    };

    SetDllDirectoryW(wpath.c_str());
    std::wstring dxcompiler = wpath + L"/dxcompiler.dll";
    HRESULT hr = m_dll_support.InitializeForDll(dxcompiler.c_str(), "DxcCreateInstance");
    return SUCCEEDED(hr);
}

DXCLoader::~DXCLoader()
{
    m_dll_support.Detach();
}

HRESULT DXCLoader::CreateInstance(REFCLSID clsid, REFIID riid, void** pResult)
{
    return m_dll_support.CreateInstance(clsid, riid, (IUnknown**)pResult);
}
