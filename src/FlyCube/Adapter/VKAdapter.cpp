#include "Adapter/VKAdapter.h"

#include "Device/VKDevice.h"

VKAdapter::VKAdapter(VKInstance& instance, const vk::PhysicalDevice& physical_device)
    : m_instance(instance)
    , m_physical_device(physical_device)
    , m_name(physical_device.getProperties().deviceName.data())
{
}

const std::string& VKAdapter::GetName() const
{
    return m_name;
}

std::shared_ptr<Device> VKAdapter::CreateDevice()
{
    return std::make_shared<VKDevice>(*this);
}

VKInstance& VKAdapter::GetInstance()
{
    return m_instance;
}

vk::PhysicalDevice& VKAdapter::GetPhysicalDevice()
{
    return m_physical_device;
}
