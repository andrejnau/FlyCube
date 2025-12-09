#include "Utilities/Common.h"

#include <cassert>

#if defined(_WIN32)
#include <Windows.h>
#endif

uint64_t Align(uint64_t size, uint64_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);
    return (size + (alignment - 1)) & ~(alignment - 1);
}

#if defined(ENABLE_VALIDATION)
bool IsValidationEnabled()
{
#if defined(_WIN32)
    return IsDebuggerPresent();
#else
    return true;
#endif
}
#else
bool IsValidationEnabled()
{
    return false;
}
#endif
