#include "AppBox/ArgsParser.h"
#include <string>

Settings ParseArgs(int argc, char* argv[])
{
    Settings settings;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--dx12")
            settings.api_type = ApiType::kDX12;
        else if (arg == "--vk")
            settings.api_type = ApiType::kVulkan;
        else if (arg == "--no_vsync")
            settings.vsync = false;
        else if (arg == "--round_fps")
            settings.round_fps = true;
        else if (arg == "--gpu")
            settings.required_gpu_index = std::stoul(argv[++i]);
    }
    return settings;
}
