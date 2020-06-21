#pragma once
#include "Device/Device.h"
#include <deque>
#include <tuple>

class DeviceBase : public Device
{
public:
    void OnDestroy();
    virtual void ExecuteCommandListsImpl(const std::vector<std::shared_ptr<CommandList>>& command_lists) = 0;
    void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists) final override;

private:
    std::vector<std::shared_ptr<CommandList>> m_command_list_pool;
    std::deque<std::pair<uint64_t /*fence_value*/, size_t /*offset*/>> m_fence_value_by_cmd;
    uint64_t m_fence_value = 0;
    std::shared_ptr<Fence> m_fence;
};
