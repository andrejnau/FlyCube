#include "Resource/ResourceStateTracker.h"

#include "Resource/Resource.h"

ResourceStateTracker::ResourceStateTracker(Resource& resource)
    : m_resource(resource)
{
}

bool ResourceStateTracker::HasResourceState() const
{
    return m_subresource_states.empty();
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
    if (it != m_subresource_states.end()) {
        return it->second;
    }
    return m_resource_state;
}

void ResourceStateTracker::SetSubresourceState(uint32_t mip_level, uint32_t array_layer, ResourceState state)
{
    if (HasResourceState() && GetResourceState() == state) {
        return;
    }
    std::tuple<uint32_t, uint32_t> key = { mip_level, array_layer };
    if (m_subresource_states.count(key)) {
        if (--m_subresource_state_groups[m_subresource_states[key]] == 0) {
            m_subresource_state_groups.erase(m_subresource_states[key]);
        }
    }
    m_subresource_states[key] = state;
    ++m_subresource_state_groups[state];
    if (m_subresource_state_groups.size() == 1 &&
        m_subresource_state_groups.begin()->second == m_resource.GetLayerCount() * m_resource.GetLevelCount()) {
        m_subresource_state_groups.clear();
        m_subresource_states.clear();
        m_resource_state = state;
    }
}

void ResourceStateTracker::Merge(const ResourceStateTracker& other)
{
    if (other.HasResourceState()) {
        auto state = other.GetResourceState();
        if (state != ResourceState::kUnknown) {
            SetResourceState(state);
        }
    } else {
        for (uint32_t i = 0; i < other.m_resource.GetLevelCount(); ++i) {
            for (uint32_t j = 0; j < other.m_resource.GetLayerCount(); ++j) {
                auto state = other.GetSubresourceState(i, j);
                if (state != ResourceState::kUnknown) {
                    SetSubresourceState(i, j, state);
                }
            }
        }
    }
}
