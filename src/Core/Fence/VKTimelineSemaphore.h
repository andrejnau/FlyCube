#pragma once
#include "Fence.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKTimelineSemaphore : public Fence {
public:
    VKTimelineSemaphore(VKDevice& device, uint64_t initial_value);
    uint64_t GetCompletedValue() override;
    void Wait(uint64_t value) override;
    void Signal(uint64_t value) override;

    const vk::Semaphore& GetFence() const;

private:
    VKDevice& m_device;
    vk::UniqueSemaphore m_timeline_semaphore;
};
