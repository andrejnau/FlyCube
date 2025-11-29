#pragma once
#include "BindingSet/BindingSetBase.h"

#import <Metal/Metal.h>

#include <map>
#include <memory>
#include <set>

class MTDevice;
class MTBindingSetLayout;
class Pipeline;

class MTBindingSet : public BindingSetBase {
public:
    MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout);

    void WriteBindings(const WriteBindingsDesc& desc) override;

    void Apply(const std::map<ShaderType, id<MTL4ArgumentTable>>& argument_tables,
               const std::shared_ptr<Pipeline>& state,
               id<MTLResidencySet> residency_set);

private:
    MTDevice& device_;
    std::set<BindKey> bindless_bind_keys_;
    std::map<BindKey, std::shared_ptr<View>> direct_bindings_;
};
