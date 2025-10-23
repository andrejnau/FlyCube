#pragma once

#include "Utilities/Logging.h"

#include <filesystem>

#ifdef NDEBUG
#define CHECK_VK_RESULT(expr) (void)(expr)
#else
#define CHECK_VK_RESULT(expr)                                                                               \
    if (vk::Result result = (expr); result != vk::Result::eSuccess) {                                       \
        Logging::Println();                                                                                 \
        Logging::Println("Check failed: {} == vk::Result::eSuccess", #expr);                                \
        Logging::Println("Result: {} ({:#x})", vk::to_string(result), (int)result);                         \
        Logging::Println("Location: {}:{}", std::filesystem::path(__FILE__).filename().string(), __LINE__); \
        Logging::Println();                                                                                 \
        abort();                                                                                            \
    }
#endif
