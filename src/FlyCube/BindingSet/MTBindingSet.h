#pragma once
#include "BindingSet/BindingSet.h"

#import <Metal/Metal.h>

class Pipeline;

class MTBindingSet : public BindingSet {
public:
    virtual void Apply(id<MTLRenderCommandEncoder> render_encoder, const std::shared_ptr<Pipeline>& state) = 0;
    virtual void Apply(id<MTLComputeCommandEncoder> compute_encoder, const std::shared_ptr<Pipeline>& state) = 0;
};
