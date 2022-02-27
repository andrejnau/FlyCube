#include "FileUtility.h"

std::string GetExecutablePath()
{
#ifdef _WIN32
    char buf[MAX_PATH + 1];
    GetModuleFileNameA(nullptr, buf, sizeof(buf));
    return buf;
#else
    char buf[BUFSIZ];
    readlink("/proc/self/exe", buf, sizeof(buf));
    return buf;
#endif
}

std::string GetExecutableDir()
{
    auto path = GetExecutablePath();
    return path.substr(0, path.find_last_of("\\/"));
}

std::string GetEnv(const std::string& var)
{
    const char* res = getenv("VULKAN_SDK");
    return res ? res : "";
}
