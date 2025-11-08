#pragma once

#include "Utilities/Logging.h"

#include <filesystem>

template <typename... Args>
inline void CheckPrintMessageImpl(std::format_string<Args...> fmt, Args&&... args)
{
    Logging::Println("Message: {}", std::format(fmt, std::forward<Args>(args)...));
}

inline void CheckPrintMessageImpl() {}

#define CHECK(expr, ...)                                                                                    \
    if (!(expr)) {                                                                                          \
        Logging::Println();                                                                                 \
        Logging::Println("Check failed: {}", #expr);                                                        \
        Logging::Println("Location: {}:{}", std::filesystem::path(__FILE__).filename().string(), __LINE__); \
        CheckPrintMessageImpl(__VA_ARGS__);                                                                 \
        Logging::Println();                                                                                 \
        abort();                                                                                            \
    }

#if defined(NDEBUG)
#define DCHECK(expr, ...) (void)(expr)
#else
#define DCHECK(expr, ...)                                                                                   \
    if (!(expr)) {                                                                                          \
        Logging::Println();                                                                                 \
        Logging::Println("Check failed: {}", #expr);                                                        \
        Logging::Println("Location: {}:{}", std::filesystem::path(__FILE__).filename().string(), __LINE__); \
        CheckPrintMessageImpl(__VA_ARGS__);                                                                 \
        Logging::Println();                                                                                 \
        abort();                                                                                            \
    }
#endif
