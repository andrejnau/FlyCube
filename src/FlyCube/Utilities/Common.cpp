#include "Utilities/Common.h"

#include <cassert>

uint64_t Align(uint64_t size, uint64_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);
    return (size + (alignment - 1)) & ~(alignment - 1);
}
