#pragma once
#include "CommandList/CommandList.h"
#include "Fence/Fence.h"
#include "Instance/QueryInterface.h"

class CommandQueue : public QueryInterface {
public:
    virtual ~CommandQueue() = default;
    virtual void Wait(const std::shared_ptr<Fence>& fence, uint64_t value) = 0;
    virtual void Signal(const std::shared_ptr<Fence>& fence, uint64_t value) = 0;
    virtual void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) = 0;
};
