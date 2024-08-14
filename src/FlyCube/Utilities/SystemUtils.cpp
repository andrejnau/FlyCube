#include "Utilities/SystemUtils.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#include <nowide/convert.hpp>

#include <vector>

std::string GetExecutablePath()
{
#if defined(_WIN32)
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, std::size(buf));
    return nowide::narrow(buf);
#elif defined(__APPLE__)
    uint32_t buf_size = 0;
    if (_NSGetExecutablePath(nullptr, &buf_size) != -1) {
        return {};
    }
    std::vector<char> buf(buf_size);
    if (_NSGetExecutablePath(buf.data(), &buf_size) != 0) {
        return {};
    }
    return buf.data();
#else
    char buf[PATH_MAX] = {};
    if (readlink("/proc/self/exe", buf, std::size(buf) - 1) == -1) {
        return {};
    }
    return buf;
#endif
}

std::string GetExecutableDir()
{
    auto path = GetExecutablePath();
    return path.substr(0, path.find_last_of("\\/"));
}

std::string GetEnvironmentVar(const std::string& name)
{
#ifdef _WIN32
    std::wstring name_utf16 = nowide::widen(name);
    DWORD value_size = GetEnvironmentVariableW(name_utf16.c_str(), nullptr, 0);
    if (value_size == 0) {
        return {};
    }

    std::vector<wchar_t> value_buffer(value_size);
    if (GetEnvironmentVariableW(name_utf16.c_str(), value_buffer.data(), value_size) != value_size - 1) {
        return {};
    }

    return nowide::narrow(value_buffer.data());
#else
    const char* res = getenv(name.c_str());
    return res ? res : "";
#endif
}
