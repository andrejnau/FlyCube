#include "CommandQueue/MTCommandQueue.h"

#include "CommandList/MTCommandList.h"
#include "CommandList/RecordCommandList.h"
#include "Device/MTDevice.h"
#include "Fence/MTFence.h"
#include "Instance/MTInstance.h"

MTCommandQueue::MTCommandQueue(MTDevice& device)
    : device_(device)
{
    command_queue_ = [device.GetDevice() newMTL4CommandQueue];
    [command_queue_ addResidencySet:device_.GetBindlessArgumentBuffer().GetResidencySet()];
}

void MTCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    [command_queue_ waitForEvent:mt_fence.GetSharedEvent() value:value];
}

void MTCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    [command_queue_ signalEvent:mt_fence.GetSharedEvent() value:value];
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

    [device_.GetBindlessArgumentBuffer().GetResidencySet() commit];
    [command_queue_ commit:command_buffers.data() count:command_buffers.size()];
}

id<MTL4CommandQueue> MTCommandQueue::GetCommandQueue()
{
    return command_queue_;
}
