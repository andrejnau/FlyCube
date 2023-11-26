#pragma once

#include <directx/dxgiformat.h>
#include <gli/gli.hpp>

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void GetSurfaceInfo(size_t width,
                    size_t height,
                    gli::format format,
                    size_t* outNumBytes,
                    size_t* outRowBytes,
                    size_t* outNumRows);

DXGI_FORMAT MakeTypelessDepthStencil(DXGI_FORMAT format);
bool IsTypelessDepthStencil(DXGI_FORMAT format);
DXGI_FORMAT DepthReadFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT StencilReadFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT DepthStencilFromTypeless(DXGI_FORMAT format);
