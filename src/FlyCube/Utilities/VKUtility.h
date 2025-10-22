#pragma once

#include "Utilities/Logging.h"

#include <format>

#ifdef NDEBUG
#define ASSERT_SUCCEEDED(expr) (void)(expr)
#else
#define ASSERT_SUCCEEDED(expr)                                                  \
    if (VkResult result = static_cast<VkResult>(expr); result != VK_SUCCESS) {  \
        Logging::Print(std::format("\nFailed in " __FILE__ ":{}\n", __LINE__)); \
        Logging::Print(std::format("--> {:#x}\n", (int)result));                \
        assert(false);                                                          \
    }
#endif
