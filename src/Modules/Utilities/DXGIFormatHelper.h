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

DXGI_FORMAT MakeTypelessDepthStencil(DXGI_FORMAT format);
bool IsTypelessDepthStencil(DXGI_FORMAT format);
DXGI_FORMAT DepthReadFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT StencilReadFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT DepthStencilFromTypeless(DXGI_FORMAT format);
