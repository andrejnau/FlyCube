#include "Adapter/MTAdapter.h"

#include "Device/MTDevice.h"
#include "Instance/MTInstance.h"

MTAdapter::MTAdapter(MTInstance& instance, id<MTLDevice> device)
    : instance_(instance)
    , device_(device)
    , name_([[device name] UTF8String])
{
}

const std::string& MTAdapter::GetName() const
{
    return name_;
}

std::shared_ptr<Device> MTAdapter::CreateDevice()
{
    return std::make_shared<MTDevice>(instance_, device_);
}
