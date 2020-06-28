#pragma once
#include "CommandQueue/CommandQueueBase.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKCommandQueue : public CommandQueueBase
{
public:
    VKCommandQueue(VKDevice& device, CommandListType type, uint32_t queue_family_index);
    ~VKCommandQueue();
    void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void ExecuteCommandListsImpl(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

    VKDevice& GetDevice();
    uint32_t GetQueueFamilyIndex();
    vk::Queue GetQueue();

private:
    VKDevice& m_device;
    uint32_t m_queue_family_index;
    vk::Queue m_queue;
};
