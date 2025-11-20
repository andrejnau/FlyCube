#include "Fence/MTFence.h"

#include "Device/MTDevice.h"

MTFence::MTFence(MTDevice& device, uint64_t initial_value)
    : device_(device)
{
    shared_event_ = [device_.GetDevice() newSharedEvent];
    shared_event_.signaledValue = initial_value;
}

uint64_t MTFence::GetCompletedValue()
{
    return shared_event_.signaledValue;
}

void MTFence::Wait(uint64_t value)
{
    while (GetCompletedValue() < value) {
        [shared_event_ waitUntilSignaledValue:value timeoutMS:10];
    }
}

void MTFence::Signal(uint64_t value)
{
    shared_event_.signaledValue = value;
}

id<MTLSharedEvent> MTFence::GetSharedEvent()
{
    return shared_event_;
}
