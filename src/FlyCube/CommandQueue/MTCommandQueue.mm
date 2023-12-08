#include "CommandQueue/MTCommandQueue.h"

#include "CommandList/MTCommandList.h"
#include "Device/MTDevice.h"
#include "Instance/MTInstance.h"

MTCommandQueue::MTCommandQueue(MTDevice& device)
    : m_device(device)
{
    m_command_queue = [device.GetDevice() newCommandQueue];
}

void MTCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value) {}

void MTCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value) {}

void MTCommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists)
{
    for (auto& command_list : command_lists) {
        if (!command_list) {
            continue;
        }
        decltype(auto) mt_command_list = command_list->As<MTCommandList>();
        mt_command_list.OnSubmit();
        [mt_command_list.GetCommandBuffer() commit];
    }
}

id<MTLCommandQueue> MTCommandQueue::GetCommandQueue()
{
    return m_command_queue;
}
