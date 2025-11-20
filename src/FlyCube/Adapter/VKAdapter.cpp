#include "Adapter/VKAdapter.h"

#include "Device/VKDevice.h"

VKAdapter::VKAdapter(VKInstance& instance, const vk::PhysicalDevice& physical_device)
    : instance_(instance)
    , physical_device_(physical_device)
    , name_(physical_device.getProperties().deviceName.data())
{
}

const std::string& VKAdapter::GetName() const
{
    return name_;
}

std::shared_ptr<Device> VKAdapter::CreateDevice()
{
    return std::make_shared<VKDevice>(*this);
}

VKInstance& VKAdapter::GetInstance()
{
    return instance_;
}

vk::PhysicalDevice& VKAdapter::GetPhysicalDevice()
{
    return physical_device_;
}
