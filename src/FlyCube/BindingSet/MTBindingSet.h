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

private:
    MTDevice& m_device;
    std::shared_ptr<MTBindingSetLayout> m_layout;
    std::map<std::pair<ShaderType, uint32_t>, id<MTLBuffer>> m_argument_buffers;
    std::map<std::pair<ShaderType, uint32_t>, uint32_t> m_slots_count;
    std::map<std::pair<ShaderType, MTLResourceUsage>, std::vector<id<MTLResource>>> m_resouces;
    std::vector<BindKey> m_direct_bind_keys;
    std::vector<BindingDesc> m_direct_bindings;
};
