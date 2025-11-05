#pragma once
#include "BindingSet/BindingSet.h"

#import <Metal/Metal.h>

class MTDevice;
class MTBindingSetLayout;
class Pipeline;

class MTBindingSet : public BindingSet {
public:
    MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;

    void Apply(const std::map<ShaderType, id<MTL4ArgumentTable>>& argument_tables,
               const std::shared_ptr<Pipeline>& state);
    void AddResourcesToResidencySet(id<MTLResidencySet> residency_set);

private:
    MTDevice& m_device;
    std::vector<id<MTLResource>> m_resources;
    std::vector<BindKey> m_direct_bind_keys;
    std::vector<BindingDesc> m_direct_bindings;
};
