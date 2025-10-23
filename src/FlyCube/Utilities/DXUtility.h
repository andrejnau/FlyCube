#pragma once

#include "Utilities/Logging.h"

#include <filesystem>

#ifdef NDEBUG
#define ASSERT_SUCCEEDED(expr) (void)(expr)
#else
#define ASSERT_SUCCEEDED(expr)                                                                              \
    if (HRESULT hr = (expr); FAILED(hr)) {                                                                  \
        Logging::Println();                                                                                 \
        Logging::Println("Assertion failed: {} >= 0", #expr);                                               \
        Logging::Println("Result: {}", hr);                                                                 \
        Logging::Println("Location: {}:{}", std::filesystem::path(__FILE__).filename().string(), __LINE__); \
        Logging::Println();                                                                                 \
        abort();                                                                                            \
    }
#endif
