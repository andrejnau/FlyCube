#pragma once
#include "Resource/MTResource.h"
#include "Utilities/PassKey.h"

#import <Metal/Metal.h>

class MTDevice;

class MTAccelerationStructure : public MTResource {
public:
    MTAccelerationStructure(PassKey<MTAccelerationStructure> pass_key, MTDevice& device);

    static std::shared_ptr<MTAccelerationStructure> CreateAccelerationStructure(MTDevice& device,
                                                                                const AccelerationStructureDesc& desc);

    // Resource:
    uint64_t GetAccelerationStructureHandle() const override;
    void SetName(const std::string& name) override;

    // MTResource:
    id<MTLAccelerationStructure> GetAccelerationStructure() const override;

private:
    MTDevice& device_;

    id<MTLAccelerationStructure> acceleration_structure_ = nullptr;
};
