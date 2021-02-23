#pragma once
#include <dxc/Support/WinIncludes.h>
#include <dxc/Support/dxcapi.use.h>
#include <string>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXCLoader
{
public:
    DXCLoader(bool dxil_required = true);
    ~DXCLoader();

    HRESULT CreateInstance(REFCLSID clsid, REFIID riid, _Outptr_ void** pResult);

private:
    bool Load(const std::string& path, bool dxil_required);

    dxc::DxcDllSupport m_dll_support;
};
