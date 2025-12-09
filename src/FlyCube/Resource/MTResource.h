#pragma once
#include "Resource/ResourceBase.h"

#import <Metal/Metal.h>

class MTResource : public ResourceBase {
public:
    virtual id<MTLTexture> GetTexture() const;
    virtual id<MTLBuffer> GetBuffer() const;
    virtual id<MTLSamplerState> GetSampler() const;
    virtual id<MTLAccelerationStructure> GetAccelerationStructure() const;
};
