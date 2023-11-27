#pragma once
#include "Instance/BaseTypes.h"

#include <functional>
#include <map>

class Resource;

class ResourceStateTracker {
public:
    ResourceStateTracker(Resource& resource);
    bool HasResourceState() const;
    ResourceState GetResourceState() const;
    void SetResourceState(ResourceState state);
    ResourceState GetSubresourceState(uint32_t mip_level, uint32_t array_layer) const;
    void SetSubresourceState(uint32_t mip_level, uint32_t array_layer, ResourceState state);
    void Merge(const ResourceStateTracker& other);

private:
    Resource& m_resource;
    std::map<std::tuple<uint32_t, uint32_t>, ResourceState> m_subresource_states;
    std::map<ResourceState, size_t> m_subresource_state_groups;
    ResourceState m_resource_state = ResourceState::kUnknown;
};
