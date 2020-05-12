#pragma once
#include "Utilities/Singleton.h"
#include <string>

struct CurState : public Singleton<CurState>
{
    bool vsync;
    bool force_dxil;
    bool round_fps;
    bool use_timeline_semaphore;
    uint32_t required_gpu_index;
    std::string gpu_name;

    CurState();
};
