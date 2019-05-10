#include "Shader/DXCLoader.h"
#include <Utilities/FileUtility.h>

DXCLoader::DXCLoader()
{
#ifdef _WIN64
    std::wstring dxcompiler = utf8_to_wstring(SDKBIN) + L"/x64/dxcompiler.dll";
#else
    std::wstring dxcompiler = utf8_to_wstring(SDKBIN) + L"/x86/dxcompiler.dll";
#endif
    m_dll_support.InitializeForDll(dxcompiler.c_str(), "DxcCreateInstance");
    m_dll_support.CreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &library);
    m_dll_support.CreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), &reflection);
    m_dll_support.CreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), &compiler);
}

DXCLoader::~DXCLoader()
{
    m_dll_support.Detach();
}
