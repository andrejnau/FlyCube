#pragma once

#include <string>

#ifdef _WIN32
#include <debugapi.h>
#else
#include <cstdio>
#endif

namespace Logging {

inline void Print(const std::string& msg)
{
#ifdef _WIN32
    OutputDebugStringA(msg.c_str());
#else
    printf("%s", msg.c_str());
#endif
}

} // namespace Logging
