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
    IFR(loader.library->CreateBlobWithEncodingOnHeapCopy(pSrcData, static_cast<UINT32>(SrcDataSize), CP_ACP, &source));
    IFR(loader.reflection->Load(source));
    IFR(loader.reflection->FindFirstPartKind(hlsl::DFCC_DXIL, &shade_idx));
    IFR(loader.reflection->GetPartReflection(shade_idx, pInterface, ppReflector));
    return S_OK;
}
