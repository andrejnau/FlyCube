#pragma once
#include "CommandList/CommandList.h"

struct ResourceBarrierManualDesc
{
    std::shared_ptr<Resource> resource;
    ResourceState state_before;
    ResourceState state_after;
    uint32_t base_mip_level = 0;
    uint32_t level_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t layer_count = 1;
};

constexpr bool kUseFakeClose = true;

class CommandListBase : public CommandList
{
public:
    virtual void ResourceBarrierManual(const std::vector<ResourceBarrierManualDesc>& barriers) = 0;
    void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) override final;
    const std::map<std::shared_ptr<Resource>, ResourceStateTracker>& GetResourceStateTrackers() const;
    const std::vector<ResourceBarrierManualDesc>& GetLazyBarriers() const;

protected:
    void OnReset();

private:
    ResourceStateTracker& GetResourceStateTracker(const std::shared_ptr<Resource>& resource);

    std::map<std::shared_ptr<Resource>, ResourceStateTracker> m_resource_state_tracker;
    std::vector<ResourceBarrierManualDesc> m_lazy_barriers;
};
