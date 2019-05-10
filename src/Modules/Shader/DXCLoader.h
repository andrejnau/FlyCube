#pragma once

#include <dxc/Support/WinIncludes.h>
#include <dxc/Support/dxcapi.use.h>

#include <wrl.h>
using namespace Microsoft::WRL;

class DXCLoader
{
public:
    DXCLoader();
    ~DXCLoader();

    ComPtr<IDxcLibrary> library;
    ComPtr<IDxcContainerReflection> reflection;
    ComPtr<IDxcCompiler> compiler;

private:
    dxc::DxcDllSupport m_dll_support;
};
