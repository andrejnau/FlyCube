#pragma once
#include <stdint.h>

inline uint32_t Align(uint32_t size, uint32_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}
