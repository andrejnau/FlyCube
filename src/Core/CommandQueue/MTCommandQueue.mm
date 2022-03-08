#include "CommandQueue/MTCommandQueue.h"

MTCommandQueue::MTCommandQueue(const id<MTLDevice>& device)
    : m_device(device)
{
    m_command_queue = [device newCommandQueue];
}

void MTCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value)
{
}

void MTCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value)
{
}

void MTCommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists)
{
}

id<MTLCommandQueue> MTCommandQueue::GetCommandQueue()
{
    return m_command_queue;
}
