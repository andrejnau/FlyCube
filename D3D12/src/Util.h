#pragma once

#include <wincodec.h>

GUID GetTargetPixelFormat(const GUID& sourceFormat);
DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID& pixelFormat);
size_t BitsPerPixel(_In_ DXGI_FORMAT fmt);