#pragma once

#include "Utilities/Singleton.h"
#include <map>

struct CurState : public Singleton<CurState>
{
    bool DXIL = false;
    bool vsync = true;
    uint32_t required_gpu_index = -1;
    std::string gpu_name;
};
