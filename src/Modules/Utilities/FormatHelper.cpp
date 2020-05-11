#include "Utilities/FormatHelper.h"
#ifdef _WIN32
#include "Utilities/DXGIFormatHelper.h"
#endif

void GetFormatInfo(
    size_t width,
    size_t height,
    gli::format format,
    size_t& num_bytes,
    size_t& row_bytes)
{
    if (gli::is_compressed(format))
    {
        row_bytes = gli::block_size(format) * ((width + gli::block_extent(format).x - 1) / gli::block_extent(format).x);
        num_bytes = row_bytes * ((height + gli::block_extent(format).y - 1) / gli::block_extent(format).y);
    }
    else
    {
        row_bytes = width * gli::detail::bits_per_pixel(format) / 8;
        num_bytes = row_bytes * height;
    }

    // validate
#if defined(_WIN32) && defined(_DEBUG)
    size_t dxgi_num_bytes = 0;
    size_t dxgi_row_bytes = 0;
    GetSurfaceInfo(width, height, format, &dxgi_num_bytes, &dxgi_row_bytes, nullptr);
    assert(dxgi_num_bytes == num_bytes);
    assert(dxgi_row_bytes == row_bytes);
#endif
}
