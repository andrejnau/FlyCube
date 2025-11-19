#pragma once

#include <gli/gli.hpp>

void GetFormatInfo(size_t width,
                   size_t height,
                   gli::format format,
                   size_t& num_bytes,
                   size_t& row_bytes,
                   size_t& num_rows,
                   uint32_t alignment);

void GetFormatInfo(size_t width,
                   size_t height,
                   gli::format format,
                   size_t& num_bytes,
                   size_t& row_bytes,
                   size_t& num_rows);

void GetFormatInfo(size_t width, size_t height, gli::format format, size_t& num_bytes, size_t& row_bytes);
