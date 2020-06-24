#include "CommandList/CommandListBase.h"
#include <Resource/Resource.h>

void CommandListBase::ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers)
{
    std::vector<ResourceBarrierManualDesc> manual_barriers;
    for (const auto& barrier : barriers)
    {
        auto& state_tracker = GetResourceStateTracker(barrier.resource);
        if (state_tracker.HasResourceState() && barrier.base_mip_level == 0 && barrier.level_count == barrier.resource->GetLevelCount() &&
            barrier.base_array_layer == 0 && barrier.layer_count == barrier.resource->GetLayerCount())
        {
            ResourceBarrierManualDesc& manual_barrier = manual_barriers.emplace_back();
            manual_barrier.resource = barrier.resource;
            manual_barrier.level_count = barrier.level_count;
            manual_barrier.layer_count = barrier.layer_count;
            manual_barrier.state_before = state_tracker.GetResourceState();
            manual_barrier.state_after = barrier.state;
            state_tracker.SetResourceState(manual_barrier.state_after);
        }
        else
        {
            for (uint32_t i = 0; i < barrier.level_count; ++i)
            {
                for (uint32_t j = 0; j < barrier.layer_count; ++j)
                {
                    ResourceBarrierManualDesc manual_barrier = {};
                    manual_barrier.resource = barrier.resource;
                    manual_barrier.base_mip_level = barrier.base_mip_level + i;
                    manual_barrier.level_count = 1;
                    manual_barrier.base_array_layer = barrier.base_array_layer + j;
                    manual_barrier.layer_count = 1;
                    manual_barrier.state_before = state_tracker.GetSubresourceState(manual_barrier.base_mip_level, manual_barrier.base_array_layer);
                    manual_barrier.state_after = barrier.state;
                    state_tracker.SetSubresourceState(manual_barrier.base_mip_level, manual_barrier.base_array_layer, manual_barrier.state_after);
                    if (manual_barrier.state_before != ResourceState::kUnknown)
                        manual_barriers.emplace_back(manual_barrier);
                    else
                        m_lazy_barriers.emplace_back(manual_barrier);
                }
            }
        }
    }
    if (!manual_barriers.empty())
        ResourceBarrierManual(manual_barriers);
}

void CommandListBase::OnReset()
{
    m_lazy_barriers.clear();
    m_resource_state_tracker.clear();
}

ResourceStateTracker& CommandListBase::GetResourceStateTracker(const std::shared_ptr<Resource>& resource)
{
    auto it = m_resource_state_tracker.find(resource);
    if (it == m_resource_state_tracker.end())
        it = m_resource_state_tracker.emplace(resource, [resource] { return resource->GetLevelCount() * resource->GetLayerCount(); }).first;
    return it->second;
}

const std::map<std::shared_ptr<Resource>, ResourceStateTracker>& CommandListBase::GetResourceStateTrackers() const
{
    return m_resource_state_tracker;
}

const std::vector<ResourceBarrierManualDesc>& CommandListBase::GetLazyBarriers() const
{
    return m_lazy_barriers;
}
