#pragma once
#include <Instance/BaseTypes.h>
#include <map>
#include <functional>

class ResourceStateTracker
{
public:
    using subresource_count_getter_t = std::function<uint32_t()>;
    ResourceStateTracker(const subresource_count_getter_t& subresource_count_getter);
    bool HasResourceState() const;
    ResourceState GetResourceState() const;
    void SetResourceState(ResourceState state);
    ResourceState GetSubresourceState(uint32_t mip_level, uint32_t array_layer) const;
    void SetSubresourceState(uint32_t mip_level, uint32_t array_layer, ResourceState state);

private:
    std::map<std::tuple<uint32_t, uint32_t>, ResourceState> m_subresource_states;
    std::map<ResourceState, size_t> m_subresource_state_groups;
    ResourceState m_resource_state = ResourceState::kUnknown;
    subresource_count_getter_t m_subresource_count_getter;
};
