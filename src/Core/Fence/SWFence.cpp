#include "Fence/SWFence.h"
#include <Device/SWDevice.h>

SWFence::SWFence(SWDevice& device, uint64_t initial_value)
    : m_device(device)
{
}

uint64_t SWFence::GetCompletedValue()
{
    return 0;
}

void SWFence::Wait(uint64_t value)
{
}

void SWFence::Signal(uint64_t value)
{
}
