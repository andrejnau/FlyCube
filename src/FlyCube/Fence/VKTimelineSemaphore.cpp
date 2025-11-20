#include "Fence/VKTimelineSemaphore.h"

#include "Device/VKDevice.h"

VKTimelineSemaphore::VKTimelineSemaphore(VKDevice& device, uint64_t initial_value)
    : device_(device)
{
    vk::SemaphoreTypeCreateInfo timeline_create_info = {};
    timeline_create_info.initialValue = initial_value;
    timeline_create_info.semaphoreType = vk::SemaphoreType::eTimeline;
    vk::SemaphoreCreateInfo create_info;
    create_info.pNext = &timeline_create_info;
    timeline_semaphore_ = device.GetDevice().createSemaphoreUnique(create_info);
}

uint64_t VKTimelineSemaphore::GetCompletedValue()
{
    return device_.GetDevice().getSemaphoreCounterValue(timeline_semaphore_.get());
}

void VKTimelineSemaphore::Wait(uint64_t value)
{
    vk::SemaphoreWaitInfo wait_info = {};
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &timeline_semaphore_.get();
    wait_info.pValues = &value;
    std::ignore = device_.GetDevice().waitSemaphores(wait_info, UINT64_MAX);
}

void VKTimelineSemaphore::Signal(uint64_t value)
{
    vk::SemaphoreSignalInfo signal_info = {};
    signal_info.semaphore = timeline_semaphore_.get();
    signal_info.value = value;
    device_.GetDevice().signalSemaphore(signal_info);
}

const vk::Semaphore& VKTimelineSemaphore::GetFence() const
{
    return timeline_semaphore_.get();
}
