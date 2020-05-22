#include "Semaphore/VKSemaphore.h"
#include <Device/VKDevice.h>

VKSemaphore::VKSemaphore(VKDevice& device)
    : m_device(device)
{
    vk::SemaphoreCreateInfo semaphore_create_info = {};
    m_semaphore = device.GetDevice().createSemaphoreUnique(semaphore_create_info);
}

const vk::Semaphore& VKSemaphore::GetSemaphore() const
{
    return m_semaphore.get();
}
