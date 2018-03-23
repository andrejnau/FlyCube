#pragma once

#include "Utilities/Singleton.h"
#include <map>

struct CurState : public SingleTon<CurState>
{
    bool DXIL = false;
    bool pause = false;
    bool no_shadow_discard = false;
    bool disable_norm = false;
};
