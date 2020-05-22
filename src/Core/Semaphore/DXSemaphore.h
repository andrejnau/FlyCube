#pragma once
#include "Semaphore/Semaphore.h"
#include <Fence/DXFence.h>

class DXDevice;

class DXSemaphore
    : public Semaphore
{
public:
    DXSemaphore(DXDevice& device);
    DXFence& GetFence();

private:
    DXFence m_fence;
};
