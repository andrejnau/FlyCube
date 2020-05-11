#pragma once
#include "Fence.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKFence : public Fence
{
public:
    VKFence(VKDevice& device);
    void WaitAndReset() override;

    const vk::Semaphore& GetFence() const;
    const uint64_t& GetValue() const;

private:
    VKDevice& m_device;
    vk::UniqueSemaphore m_timeline_semaphore;
    uint64_t m_fence_value = 0;
};
