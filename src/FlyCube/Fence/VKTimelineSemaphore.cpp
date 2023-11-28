#include "Fence/VKTimelineSemaphore.h"

#include "Device/VKDevice.h"

VKTimelineSemaphore::VKTimelineSemaphore(VKDevice& device, uint64_t initial_value)
    : m_device(device)
{
    vk::SemaphoreTypeCreateInfo timeline_create_info = {};
    timeline_create_info.initialValue = initial_value;
    timeline_create_info.semaphoreType = vk::SemaphoreType::eTimeline;
    vk::SemaphoreCreateInfo create_info;
    create_info.pNext = &timeline_create_info;
    m_timeline_semaphore = device.GetDevice().createSemaphoreUnique(create_info);
}

uint64_t VKTimelineSemaphore::GetCompletedValue()
{
    return m_device.GetDevice().getSemaphoreCounterValueKHR(m_timeline_semaphore.get());
}

void VKTimelineSemaphore::Wait(uint64_t value)
{
    vk::SemaphoreWaitInfo wait_info = {};
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &m_timeline_semaphore.get();
    wait_info.pValues = &value;
    std::ignore = m_device.GetDevice().waitSemaphoresKHR(wait_info, UINT64_MAX);
}

void VKTimelineSemaphore::Signal(uint64_t value)
{
    vk::SemaphoreSignalInfo signal_info = {};
    signal_info.semaphore = m_timeline_semaphore.get();
    signal_info.value = value;
    m_device.GetDevice().signalSemaphoreKHR(signal_info);
}

const vk::Semaphore& VKTimelineSemaphore::GetFence() const
{
    return m_timeline_semaphore.get();
}
