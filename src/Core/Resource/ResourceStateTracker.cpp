#include "Resource/ResourceStateTracker.h"

ResourceStateTracker::ResourceStateTracker(const subresource_count_getter_t& subresource_count_getter)
    : m_subresource_count_getter(subresource_count_getter)
{
}

bool ResourceStateTracker::HasResourceState() const
{
    return m_subresource_states.empty() && m_resource_state != ResourceState::kUnknown;
}

ResourceState ResourceStateTracker::GetResourceState() const
{
    return m_resource_state;
}

void ResourceStateTracker::SetResourceState(ResourceState state)
{
    m_subresource_states.clear();
    m_resource_state = state;
    m_subresource_state_groups.clear();
}

ResourceState ResourceStateTracker::GetSubresourceState(uint32_t mip_level, uint32_t array_layer) const
{
    auto it = m_subresource_states.find({ mip_level, array_layer });
    if (it != m_subresource_states.end())
        return it->second;
    return m_resource_state;
}

void ResourceStateTracker::SetSubresourceState(uint32_t mip_level, uint32_t array_layer, ResourceState state)
{
    if (HasResourceState() && GetResourceState() == state)
        return;
    std::tuple<uint32_t, uint32_t> key = { mip_level, array_layer };
    if (m_subresource_states.count(key))
    {
        if (--m_subresource_state_groups[m_subresource_states[key]] == 0)
            m_subresource_state_groups.erase(m_subresource_states[key]);
    }
    m_subresource_states[key] = state;
    ++m_subresource_state_groups[state];
    if (m_subresource_state_groups.size() == 1 && m_subresource_state_groups.begin()->second == m_subresource_count_getter())
    {
        m_subresource_state_groups.clear();
        m_subresource_states.clear();
        m_resource_state = state;
    }
}
