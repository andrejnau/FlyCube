#pragma once
#include "BindingSet/BindingSet.h"

#import <Metal/Metal.h>

#include <map>
#include <memory>
#include <set>

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
    std::set<BindKey> m_bindless_bind_keys;
    std::map<BindKey, std::shared_ptr<View>> m_direct_bindings;
};
