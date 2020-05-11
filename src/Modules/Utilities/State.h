#pragma once
#include "Utilities/Singleton.h"
#include <string>

struct CurState : public Singleton<CurState>
{
    CurState();

    bool vsync;
    bool force_dxil;
    bool round_fps;
    uint32_t required_gpu_index;
    std::string gpu_name;
};
