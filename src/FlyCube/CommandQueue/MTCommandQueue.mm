#include "CommandQueue/MTCommandQueue.h"

#include "CommandList/MTCommandList.h"
#include "Device/MTDevice.h"
#include "Fence/MTFence.h"
#include "Instance/MTInstance.h"

MTCommandQueue::MTCommandQueue(MTDevice& device)
    : m_device(device)
{
    m_command_queue = [device.GetDevice() newMTL4CommandQueue];
    [m_command_queue addResidencySet:m_device.GetResidencySet()];
}

void MTCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    [m_command_queue waitForEvent:mt_fence.GetSharedEvent() value:value];
}

void MTCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    [m_command_queue signalEvent:mt_fence.GetSharedEvent() value:value];
}

void MTCommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists)
{
    [m_device.GetResidencySet() commit];
    for (auto& command_list : command_lists) {
        if (!command_list) {
            continue;
        }
        decltype(auto) mt_command_list = command_list->As<MTCommandList>();
        mt_command_list.OnSubmit();
        auto commandBuffer = mt_command_list.GetCommandBuffer();
        [m_command_queue commit:&commandBuffer count:1];
    }
}

id<MTL4CommandQueue> MTCommandQueue::GetCommandQueue()
{
    return m_command_queue;
}
