#pragma once
#include "Adapter.h"

#import <Metal/Metal.h>

class MTInstance;

class MTAdapter : public Adapter {
public:
    MTAdapter(MTInstance& instance, id<MTLDevice> device);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;

private:
    MTInstance& instance_;
    const id<MTLDevice> device_;
    std::string name_;
};
