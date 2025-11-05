#pragma once

#include <format>
#include <string>

#if defined(__OBJC__)
#include "Utilities/ObjcFormatter.h"
#endif

namespace Logging {

void PrintImpl(const std::string& str);

template <typename... Args>
inline void Print(std::format_string<Args...> fmt, Args&&... args)
{
    PrintImpl(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
inline void Println(std::format_string<Args...> fmt, Args&&... args)
{
    PrintImpl(std::format(fmt, std::forward<Args>(args)...).append("\n"));
}

inline void Println()
{
    PrintImpl("\n");
}

} // namespace Logging
