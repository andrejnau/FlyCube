#pragma once
#include <stdint.h>

inline uint64_t Align(uint64_t size, uint64_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}
