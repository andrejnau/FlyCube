#pragma once
#include "CommandQueue/CommandQueue.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKCommandQueue : public CommandQueue {
public:
    VKCommandQueue(VKDevice& device, CommandListType type, uint32_t queue_family_index);
    void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

    VKDevice& GetDevice();
    uint32_t GetQueueFamilyIndex();
    vk::Queue GetQueue();

private:
    VKDevice& device_;
    uint32_t queue_family_index_;
    vk::Queue queue_;
};
