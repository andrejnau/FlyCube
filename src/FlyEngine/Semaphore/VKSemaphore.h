#pragma once
#include "Semaphore.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKSemaphore : public Semaphore
{
public:
    VKSemaphore(VKDevice& device);

    const vk::Semaphore& GetSemaphore() const;

private:
    VKDevice& m_device;
    vk::UniqueSemaphore m_semaphore;
};
