#pragma once

#include <directx/dxgiformat.h>

DXGI_FORMAT MakeTypelessDepthStencil(DXGI_FORMAT format);
bool IsTypelessDepthStencil(DXGI_FORMAT format);
DXGI_FORMAT DepthReadFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT StencilReadFromTypeless(DXGI_FORMAT format);
DXGI_FORMAT DepthStencilFromTypeless(DXGI_FORMAT format);
