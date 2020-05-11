#pragma once
#include "Semaphore/Semaphore.h"
#include <Fence/DXFence.h>

class DXSemaphore
    : public Semaphore
    , public DXFence
{
public:
    using DXFence::DXFence;
    void Increment();
};
