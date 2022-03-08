#pragma once
#include "CommandQueue/CommandQueue.h"
#include <Device/MTDevice.h>

class MTCommandQueue : public CommandQueue
{
public:
    MTCommandQueue(const id<MTLDevice>& device);
    void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

    id<MTLCommandQueue> GetCommandQueue();

private:
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_command_queue;
};
