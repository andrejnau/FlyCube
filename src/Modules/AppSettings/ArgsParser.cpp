#include "AppSettings/ArgsParser.h"

#include <string>
#include <vector>

namespace {

std::vector<ApiType> GetSupportedApis()
{
    std::vector<ApiType> res;
#ifdef DIRECTX_SUPPORT
    res.emplace_back(ApiType::kDX12);
#endif
#ifdef METAL_SUPPORT
    res.emplace_back(ApiType::kMetal);
#endif
#ifdef VULKAN_SUPPORT
    res.emplace_back(ApiType::kVulkan);
#endif
    return res;
}

} // namespace

Settings ParseArgs(int argc, char* argv[])
{
    Settings settings;
    settings.api_type = GetSupportedApis()[0];
    settings.vsync = true;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--dx12") {
            settings.api_type = ApiType::kDX12;
        } else if (arg == "--vk") {
            settings.api_type = ApiType::kVulkan;
        } else if (arg == "--mt") {
            settings.api_type = ApiType::kMetal;
        } else if (arg == "--vsync") {
            settings.vsync = true;
        } else if (arg == "--no_vsync") {
            settings.vsync = false;
        } else if (arg == "--gpu") {
            settings.required_gpu_index = std::stoul(argv[++i]);
        }
    }
    return settings;
}
