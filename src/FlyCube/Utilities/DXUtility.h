#pragma once

#include "Utilities/Logging.h"

#include <filesystem>

#if defined(NDEBUG)
#define CHECK_HRESULT(expr) (void)(expr)
#else
#define CHECK_HRESULT(expr)                                                                                 \
    if (HRESULT hr = (expr); FAILED(hr)) {                                                                  \
        Logging::Println();                                                                                 \
        Logging::Println("Check failed: {} >= 0", #expr);                                                   \
        Logging::Println("Result: {}", hr);                                                                 \
        Logging::Println("Location: {}:{}", std::filesystem::path(__FILE__).filename().string(), __LINE__); \
        Logging::Println();                                                                                 \
        abort();                                                                                            \
    }
#endif
