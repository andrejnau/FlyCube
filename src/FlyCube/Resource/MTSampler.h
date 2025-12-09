#pragma once
#include "Resource/MTResource.h"
#include "Utilities/PassKey.h"

#import <Metal/Metal.h>

class MTDevice;

class MTSampler : public MTResource {
public:
    MTSampler(PassKey<MTSampler> pass_key, MTDevice& device);

    static std::shared_ptr<MTSampler> CreateSampler(MTDevice& device, const SamplerDesc& desc);

    // Resource:
    void SetName(const std::string& name) override;

    // MTResource:
    id<MTLSamplerState> GetSampler() const override;

private:
    MTDevice& device_;

    id<MTLSamplerState> sampler_ = nullptr;
};
