#pragma once

#include "Utilities/Singleton.h"
#include <map>

struct CurState : public Singleton<CurState>
{
    bool DXIL = false;
    std::string gpu_name;
};
