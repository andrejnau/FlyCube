#include "Adapter/MTAdapter.h"
#include <Device/MTDevice.h>

MTAdapter::MTAdapter(const id<MTLDevice>& device)
    : m_device(device)
    , m_name([[device name] UTF8String])
{
}

const std::string& MTAdapter::GetName() const
{
    return m_name;
}

std::shared_ptr<Device> MTAdapter::CreateDevice()
{
    return std::make_shared<MTDevice>(m_device);
}
