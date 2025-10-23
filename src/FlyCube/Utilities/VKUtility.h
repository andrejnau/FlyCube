#pragma once

#include "Utilities/Logging.h"

#include <filesystem>

#ifdef NDEBUG
#define ASSERT_SUCCEEDED(expr) (void)(expr)
#else
#define ASSERT_SUCCEEDED(expr)                                                                              \
    if (vk::Result result = (expr); result != vk::Result::eSuccess) {                                       \
        Logging::Println();                                                                                 \
        Logging::Println("Assertion failed: {} == vk::Result::eSuccess", #expr);                            \
        Logging::Println("Result: {} ({:#x})", vk::to_string(result), (int)result);                         \
        Logging::Println("Location: {}:{}", std::filesystem::path(__FILE__).filename().string(), __LINE__); \
        Logging::Println();                                                                                 \
        abort();                                                                                            \
    }
#endif
