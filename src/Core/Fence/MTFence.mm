#include "Fence/MTFence.h"

#include "Device/MTDevice.h"

MTFence::MTFence(MTDevice& device, uint64_t initial_value)
    : m_device(device)
{
}

uint64_t MTFence::GetCompletedValue()
{
    return 0;
}

void MTFence::Wait(uint64_t value) {}

void MTFence::Signal(uint64_t value) {}
