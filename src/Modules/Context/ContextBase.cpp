#include "Context/ContextBase.h"
#include <CommandListBox/CommandListBox.h>
#include <Device/Device.h>

void ContextBase::OnDestroy()
{
    if (m_fence)
    {
        m_fence->Wait(m_fence_value);
        m_fence.reset();
    }
    m_command_list_pool.clear();
}

void ContextBase::ExecuteCommandListsImpl(const std::vector<std::shared_ptr<CommandListBox>>& command_lists)
{
    if (!m_fence)
        m_fence = m_device->CreateFence(m_fence_value);

    std::vector<std::shared_ptr<CommandList>> raw_command_lists;
    size_t patch_cmds = 0;
    for (size_t c = 0; c < command_lists.size(); ++c)
    {
        std::vector<ResourceBarrierDesc> new_barriers;
        auto& command_list_base = *command_lists[c];
        auto barriers = command_list_base.GetLazyBarriers();
        for (auto& barrier : barriers)
        {
            if (c == 0 && barrier.resource->AllowCommonStatePromotion(barrier.state_after))
                continue;
            auto& global_state_tracker = barrier.resource->GetGlobalResourceStateTracker();
            if (global_state_tracker.HasResourceState() && barrier.base_mip_level == 0 && barrier.level_count == barrier.resource->GetLevelCount() &&
                barrier.base_array_layer == 0 && barrier.layer_count == barrier.resource->GetLayerCount())
            {
                barrier.state_before = global_state_tracker.GetResourceState();
                if (barrier.state_before != barrier.state_after)
                    new_barriers.emplace_back(barrier);
            }
            else
            {
                for (uint32_t i = 0; i < barrier.level_count; ++i)
                {
                    for (uint32_t j = 0; j < barrier.layer_count; ++j)
                    {
                        barrier.state_before = global_state_tracker.GetSubresourceState(barrier.base_mip_level + i, barrier.base_array_layer + j);
                        if (barrier.state_before != barrier.state_after)
                        {
                            auto& new_barrier = new_barriers.emplace_back(barrier);
                            new_barrier.base_mip_level = barrier.base_mip_level + i;
                            new_barrier.level_count = 1;
                            new_barrier.base_array_layer = barrier.base_array_layer + j;
                            new_barrier.layer_count = 1;
                        }
                    }
                }
            }
        }

        if (!new_barriers.empty())
        {
            std::shared_ptr<CommandList> tmp_cmd;
            if (c != 0 && kUseFakeClose)
            {
                tmp_cmd = command_lists[c - 1]->GetCommandList();
            }
            else
            {
                if (!m_fence_value_by_cmd.empty())
                {
                    auto& desc = m_fence_value_by_cmd.front();
                    if (m_fence->GetCompletedValue() >= desc.first)
                    {
                        m_fence->Wait(desc.first);
                        tmp_cmd = m_command_list_pool[desc.second];
                        tmp_cmd->Reset();
                        m_fence_value_by_cmd.pop_front();
                        m_fence_value_by_cmd.emplace_back(m_fence_value + 1, desc.second);
                    }
                }
                if (!tmp_cmd)
                {
                    tmp_cmd = m_command_list_pool.emplace_back(m_device->CreateCommandList(m_type));
                    m_fence_value_by_cmd.emplace_back(m_fence_value + 1, m_command_list_pool.size() - 1);
                }
                raw_command_lists.emplace_back(tmp_cmd);
            }

            tmp_cmd->ResourceBarrier(new_barriers);
            if (!kUseFakeClose)
                tmp_cmd->Close();
            ++patch_cmds;
        }

        auto& state_trackers = command_list_base.GetResourceStateTrackers();
        for (const auto& state_tracker_pair : state_trackers)
        {
            auto& resource = state_tracker_pair.first;
            auto& state_tracker = state_tracker_pair.second;
            auto& global_state_tracker = resource->GetGlobalResourceStateTracker();
            global_state_tracker.Merge(state_tracker);
        }

        raw_command_lists.emplace_back(command_lists[c]->GetCommandList());
    }
    if (kUseFakeClose)
    {
        for (auto& cmd : raw_command_lists)
            cmd->Close();
    }
    m_command_queue->ExecuteCommandLists(raw_command_lists);
    if (patch_cmds)
        m_command_queue->Signal(m_fence, ++m_fence_value);
}
