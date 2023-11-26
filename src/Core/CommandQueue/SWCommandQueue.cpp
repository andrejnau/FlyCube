#include "CommandQueue/SWCommandQueue.h"

#include "CommandList/SWCommandList.h"
#include "Device/SWDevice.h"
#include "Fence/SWFence.h"

SWCommandQueue::SWCommandQueue(SWDevice& device, CommandListType type)
    : m_device(device)
{
}

void SWCommandQueue::Wait(const std::shared_ptr<Fence>& fence, uint64_t value) {}

void SWCommandQueue::Signal(const std::shared_ptr<Fence>& fence, uint64_t value) {}

void SWCommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) {}
