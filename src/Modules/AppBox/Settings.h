#pragma once
#include "ApiType/ApiType.h"

struct Settings {
    ApiType api_type = ApiType::kVulkan;
    bool vsync = true;
    uint32_t required_gpu_index = 0;
};
