#include "Fence/MTFence.h"

#include "Device/MTDevice.h"

MTFence::MTFence(MTDevice& device, uint64_t initial_value)
    : m_device(device)
{
    m_shared_event = [m_device.GetDevice() newSharedEvent];
    m_shared_event.signaledValue = initial_value;
}

uint64_t MTFence::GetCompletedValue()
{
    return m_shared_event.signaledValue;
}

void MTFence::Wait(uint64_t value)
{
    while (GetCompletedValue() < value) {
        [m_shared_event waitUntilSignaledValue:value timeoutMS:10];
    }
}

void MTFence::Signal(uint64_t value)
{
    m_shared_event.signaledValue = value;
}

id<MTLSharedEvent> MTFence::GetSharedEvent()
{
    return m_shared_event;
}
