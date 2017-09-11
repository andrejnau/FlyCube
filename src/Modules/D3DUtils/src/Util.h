#pragma once

#include <wincodec.h>

GUID GetTargetPixelFormat(const GUID& sourceFormat);
DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID& pixelFormat);
size_t BitsPerPixel(_In_ DXGI_FORMAT fmt);
void GetSurfaceInfo(_In_ size_t width,
    _In_ size_t height,
    _In_ DXGI_FORMAT fmt,
    _Out_opt_ size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows);
