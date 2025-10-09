#include "CommandQueue/MTCommandQueue.h"

#include "CommandList/MTCommandList.h"
#include "CommandList/RecordCommandList.h"
#include "Device/MTDevice.h"
#include "Fence/MTFence.h"
#include "Instance/MTInstance.h"

MTCommandQueue::MTCommandQueue(MTDevice& device)
    : m_device(device)
{
    m_command_queue = [device.GetDevice() newMTL4CommandQueue];
    auto global_residency_set = m_device.GetGlobalResidencySet();
    if (global_residency_set) {
        [m_command_queue addResidencySet:global_residency_set];
    }
    [m_command_queue addResidencySet:m_device.GetBindlessArgumentBuffer().GetResidencySet()];
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
    std::vector<id<MTL4CommandBuffer>> command_buffers;
    for (auto& command_list : command_lists) {
        if (!command_list) {
            continue;
        }
        decltype(auto) record_command_list = command_list->As<RecordCommandList<MTCommandList>>();
        command_buffers.push_back(record_command_list.OnSubmit()->GetCommandBuffer());
    }

    [m_device.GetGlobalResidencySet() commit];
    [m_device.GetBindlessArgumentBuffer().GetResidencySet() commit];
    [m_command_queue commit:command_buffers.data() count:command_buffers.size()];
}

id<MTL4CommandQueue> MTCommandQueue::GetCommandQueue()
{
    return m_command_queue;
}
