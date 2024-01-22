#include "Utilities/FormatHelper.h"

#include "Utilities/Common.h"

void GetFormatInfo(size_t width,
                   size_t height,
                   gli::format format,
                   size_t& num_bytes,
                   size_t& row_bytes,
                   size_t& num_rows,
                   uint32_t alignment)
{
    if (gli::is_compressed(format)) {
        row_bytes = gli::block_size(format) * ((width + gli::block_extent(format).x - 1) / gli::block_extent(format).x);
        row_bytes = Align(row_bytes, alignment);
        num_rows = ((height + gli::block_extent(format).y - 1) / gli::block_extent(format).y);
    } else {
        row_bytes = width * gli::detail::bits_per_pixel(format) / 8;
        row_bytes = Align(row_bytes, alignment);
        num_rows = height;
    }
    num_bytes = row_bytes * num_rows;
}

void GetFormatInfo(size_t width, size_t height, gli::format format, size_t& num_bytes, size_t& row_bytes)
{
    size_t num_rows = 0;
    uint32_t alignment = 1;
    return GetFormatInfo(width, height, format, num_bytes, row_bytes, num_rows, alignment);
}
