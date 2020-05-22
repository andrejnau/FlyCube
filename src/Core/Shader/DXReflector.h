#include <Windows.h>
#include <d3dcompiler.h>
#include <dxc/dxcapi.h>

#define D3D_SIT_RTACCELERATIONSTRUCTURE 12

HRESULT DXReflect(
    _In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSize,
    _In_ REFIID pInterface,
    _Out_ void** ppReflector);
