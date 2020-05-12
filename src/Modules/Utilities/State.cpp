#include "Utilities/State.h"

CurState::CurState()
    : vsync(true)
    , force_dxil(false)
    , round_fps(false)
    , use_timeline_semaphore(false)
    , required_gpu_index(-1)
{
}
