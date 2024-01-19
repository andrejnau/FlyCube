#include "Utilities/DXGIFormatHelper.h"

DXGI_FORMAT MakeTypelessDepthStencil(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32G8X24_TYPELESS;
    default:
        return format;
    }
}

bool IsTypelessDepthStencil(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return true;
    default:
        return false;
    }
}

DXGI_FORMAT DepthReadFromTypeless(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R16_TYPELESS:
        return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R32_TYPELESS:
        return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    default:
        return format;
    }
}

DXGI_FORMAT StencilReadFromTypeless(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    default:
        return format;
    }
}

DXGI_FORMAT DepthStencilFromTypeless(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R16_TYPELESS:
        return DXGI_FORMAT_D16_UNORM;
    case DXGI_FORMAT_R32_TYPELESS:
        return DXGI_FORMAT_D32_FLOAT;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    default:
        return format;
    }
}
