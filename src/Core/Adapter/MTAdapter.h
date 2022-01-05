#pragma once
#include "Adapter.h"
#import <Metal/Metal.h>

class MTAdapter : public Adapter
{
public:
    MTAdapter(const id<MTLDevice>& device);
    const std::string& GetName() const override;
    std::shared_ptr<Device> CreateDevice() override;

private:
    id<MTLDevice> m_device;
    std::string m_name;
};
