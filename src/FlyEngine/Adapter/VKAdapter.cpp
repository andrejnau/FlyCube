#include "Adapter/VKAdapter.h"
#include <Device/VKDevice.h>

VKAdapter::VKAdapter(const vk::PhysicalDevice& physical_device)
    : m_physical_device(physical_device)
    , m_name(physical_device.getProperties().deviceName)
{
}

const std::string& VKAdapter::GetName() const
{
    return m_name;
}

std::unique_ptr<Device> VKAdapter::CreateDevice()
{
    return std::make_unique<VKDevice>(m_physical_device);
}

vk::PhysicalDevice& VKAdapter::GetPhysicalDevice()
{
    return m_physical_device;
}
