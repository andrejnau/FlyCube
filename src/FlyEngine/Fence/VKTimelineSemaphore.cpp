#include "Fence/VKTimelineSemaphore.h"
#include <Device/VKDevice.h>

VKTimelineSemaphore::VKTimelineSemaphore(VKDevice& device)
    : m_device(device)
{
    vk::SemaphoreTypeCreateInfo timeline_create_info = {};
    timeline_create_info.semaphoreType = vk::SemaphoreType::eTimeline;
    vk::SemaphoreCreateInfo create_info;
    create_info.pNext = &timeline_create_info;
    m_timeline_semaphore = device.GetDevice().createSemaphoreUnique(create_info);
}

void VKTimelineSemaphore::WaitAndReset()
{
    vk::SemaphoreWaitInfo wait_info = {};
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &m_timeline_semaphore.get();
    wait_info.pValues = &m_fence_value;
    m_device.GetDevice().waitSemaphoresKHR(wait_info, UINT64_MAX);
    ++m_fence_value;
}

const vk::Semaphore& VKTimelineSemaphore::GetFence() const
{
    return m_timeline_semaphore.get();
}

const uint64_t& VKTimelineSemaphore::GetValue() const
{
    return m_fence_value;
}
