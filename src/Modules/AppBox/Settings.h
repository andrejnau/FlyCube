#pragma once
#include <ApiType/ApiType.h>

struct Settings
{
    ApiType api_type = ApiType::kVulkan;
    bool vsync = true;
    bool round_fps = false;
    uint32_t required_gpu_index = 0;
};
