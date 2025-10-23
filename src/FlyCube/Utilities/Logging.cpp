#include "Utilities/Logging.h"

#if defined(_WIN32)
#include <Windows.h>
#include <nowide/convert.hpp>
#elif defined(__ANDROID__)
#include <android/log.h>
#else
#include <cstdio>
#endif

namespace Logging {

void PrintImpl(const std::string& str)
{
#if defined(_WIN32)
    OutputDebugStringW(nowide::widen(str).c_str());
#elif defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_DEBUG, "FlyCube", str.c_str());
#else
    fwrite(str.data(), 1, str.size(), stdout);
#endif
}

} // namespace Logging
