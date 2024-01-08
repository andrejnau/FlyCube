#pragma once
#include "BindingSet/MTBindingSet.h"

#import <Metal/Metal.h>

class MTDevice;
class MTBindingSetLayout;
class Pipeline;

class MTArgumentBuffer : public MTBindingSet {
public:
    MTArgumentBuffer(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;

    void Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state) override;
    void Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state) override;

private:
    MTDevice& m_device;
    std::shared_ptr<MTBindingSetLayout> m_layout;
    std::vector<BindingDesc> m_bindings;
    std::map<std::pair<ShaderType, uint32_t>, id<MTLBuffer>> m_argument_buffers;
    std::map<std::pair<ShaderType, uint32_t>, uint32_t> m_slots_count;
    std::map<MTLResourceUsage, std::vector<id<MTLResource>>> m_compure_resouces;
    std::map<std::pair<MTLRenderStages, MTLResourceUsage>, std::vector<id<MTLResource>>> m_graphics_resouces;
    std::vector<BindKey> m_direct_bind_keys;
    std::vector<BindingDesc> m_direct_bindings;
};
