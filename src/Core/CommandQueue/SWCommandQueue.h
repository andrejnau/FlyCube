#pragma once
#include "CommandQueue/CommandQueue.h"

class SWDevice;

class SWCommandQueue : public CommandQueue {
public:
    SWCommandQueue(SWDevice& device, CommandListType type);
    void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) override;

private:
    SWDevice& m_device;
};
