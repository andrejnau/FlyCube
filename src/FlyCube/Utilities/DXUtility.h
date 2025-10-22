#pragma once

#include "Utilities/Logging.h"

#include <format>

#ifdef NDEBUG
#define ASSERT_SUCCEEDED(expr) (void)(expr)
#else
#define ASSERT_SUCCEEDED(expr)                                                  \
    if (HRESULT hr = (expr); FAILED(hr)) {                                      \
        Logging::Print(std::format("\nFailed in " __FILE__ ":{}\n", __LINE__)); \
        Logging::Print(std::format("--> {:#x}\n", hr));                         \
        assert(false);                                                          \
    }
#endif
