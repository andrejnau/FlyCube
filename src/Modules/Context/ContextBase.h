#pragma once
#include "CommandQueue/CommandQueue.h"
#include <deque>
#include <tuple>

class Device;

class ContextBase
{
public:
    void OnDestroy();
    void ExecuteCommandListsImpl(const std::vector<std::shared_ptr<CommandListBox>>& command_lists);

protected:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandQueue> m_command_queue;

private:
    CommandListType m_type = CommandListType::kGraphics;
    std::vector<std::shared_ptr<CommandList>> m_command_list_pool;
    std::deque<std::pair<uint64_t /*fence_value*/, size_t /*offset*/>> m_fence_value_by_cmd;
    uint64_t m_fence_value = 0;
    std::shared_ptr<Fence> m_fence;
};
