#pragma once

#include "Utilities/Singleton.h"
#include <map>

struct CurState : public Singleton<CurState>
{
    bool vsync = true;
    bool force_dxil = false;
    uint32_t required_gpu_index = -1;
    std::string gpu_name;
};
