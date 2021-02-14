#pragma once

#include <gli/gli.hpp>
#include <dxgiformat.h>
#include <sal.h>

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void GetSurfaceInfo(
    _In_ size_t width,
    _In_ size_t height,
    _In_ gli::format format,
    _Out_opt_ size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows);

DXGI_FORMAT MakeTypeless(DXGI_FORMAT format);
bool IsTypeless(DXGI_FORMAT format);
DXGI_FORMAT FloatFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT UintFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT SintFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT DepthStencilFromTypeless(DXGI_FORMAT format);
