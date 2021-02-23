#include "HLSLCompiler/DXReflector.h"
#include "HLSLCompiler/DXCLoader.h"
#include <dxc/Support/Global.h>
#include <dxc/DxilContainer/DxilContainer.h>

HRESULT DXReflect(
    _In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSize,
    _In_ REFIID pInterface,
    _Out_ void** ppReflector)
{
    *ppReflector = nullptr;
    DXCLoader loader;
    CComPtr<IDxcBlobEncoding> source;
    uint32_t shade_idx = 0;
    ComPtr<IDxcLibrary> library;
    IFR(loader.CreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
    IFR(library->CreateBlobWithEncodingOnHeapCopy(pSrcData, static_cast<UINT32>(SrcDataSize), CP_ACP, &source));
    ComPtr<IDxcContainerReflection> reflection;
    IFR(loader.CreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&reflection)));
    IFR(reflection->Load(source));
    IFR(reflection->FindFirstPartKind(hlsl::DFCC_DXIL, &shade_idx));
    IFR(reflection->GetPartReflection(shade_idx, pInterface, ppReflector));
    return S_OK;
}
