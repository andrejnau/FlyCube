#pragma once
#include "Adapter.h"

#import <Metal/Metal.h>

class MTInstance;

class MTAdapter : public Adapter {
public:
    MTAdapter(MTInstance& instance, const id<MTLDevice>& device);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;

private:
    MTInstance& m_instance;
    id<MTLDevice> m_device;
    std::string m_name;
};
