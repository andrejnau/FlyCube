#include "Utilities/SystemUtils.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <stdlib.h>
#endif
#include <nowide/convert.hpp>
#include <vector>

std::string GetExecutablePath()
{
#ifdef _WIN32
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, std::size(buf));
    return nowide::narrow(buf);
#else
    char buf[PATH_MAX] = {};
    readlink("/proc/self/exe", buf, std::size(buf) - 1);
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
    if (value_size == 0)
        return {};

    std::vector<wchar_t> value_buffer(value_size);
    if (GetEnvironmentVariableW(name_utf16.c_str(), value_buffer.data(), value_size) != value_size - 1)
        return {};

    return nowide::narrow(value_buffer.data());
#else
    const char* res = getenv(name.c_str());
    return res ? res : "";
#endif
}
