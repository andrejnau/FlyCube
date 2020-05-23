#pragma once

#include <dxc/Support/WinIncludes.h>
#include <dxc/Support/dxcapi.use.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <string>

class DXCLoader
{
public:
    DXCLoader(bool dxil_required = true);
    ~DXCLoader();

    ComPtr<IDxcLibrary> library;
    ComPtr<IDxcContainerReflection> reflection;
    ComPtr<IDxcCompiler> compiler;

private:
    bool Load(const std::wstring& path, bool dxil_required);
    dxc::DxcDllSupport m_dll_support;
};
