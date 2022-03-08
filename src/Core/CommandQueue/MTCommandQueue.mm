#include "CommandQueue/MTCommandQueue.h"
#include "CommandList/MTCommandList.h"

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
    for (auto& command_list : command_lists)
    {
        if (!command_list)
            continue;
        decltype(auto) mt_command_list = command_list->As<MTCommandList>();
        [mt_command_list.GetCommandBuffer() commit];
        [mt_command_list.GetCommandBuffer() waitUntilCompleted];
    }
}

id<MTLCommandQueue> MTCommandQueue::GetCommandQueue()
{
    return m_command_queue;
}
