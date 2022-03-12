#pragma once
#include "BindingSet/BindingSet.h"
#import <Metal/Metal.h>

class MTDevice;
class MTBindingSetLayout;
class MTGraphicsPipeline;
class MTView;

class MTBindingSet
    : public BindingSet
{
public:
    MTBindingSet(MTDevice& device, const std::shared_ptr<MTBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;
    
    void Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<MTGraphicsPipeline>& state);

private:
    void SetPixelShaderView(id<MTLRenderCommandEncoder> render_encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index);
    void SetVertexShaderView(id<MTLRenderCommandEncoder> render_encoder, ViewType view_type, const std::shared_ptr<MTView>& view, uint32_t index);

    MTDevice& m_device;
    std::shared_ptr<MTBindingSetLayout> m_layout;
    std::vector<BindingDesc> m_bindings;
};
