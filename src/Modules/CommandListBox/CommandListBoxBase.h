#pragma once
#include "CommandList/CommandList.h"

struct LazyResourceBarrierDesc
{
    std::shared_ptr<Resource> resource;
    ResourceState state;
    uint32_t base_mip_level = 0;
    uint32_t level_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t layer_count = 1;
};

constexpr bool kUseFakeClose = true;

class CommandListBoxBase
{
public:
    void LazyResourceBarrier(const std::vector<LazyResourceBarrierDesc>& barriers);
    const std::map<std::shared_ptr<Resource>, ResourceStateTracker>& GetResourceStateTrackers() const;
    const std::vector<ResourceBarrierDesc>& GetLazyBarriers() const;

protected:
    void OnReset();

    std::shared_ptr<CommandList> m_command_list;

private:
    ResourceStateTracker& GetResourceStateTracker(const std::shared_ptr<Resource>& resource);

    std::map<std::shared_ptr<Resource>, ResourceStateTracker> m_resource_state_tracker;
    std::vector<ResourceBarrierDesc> m_lazy_barriers;
};
