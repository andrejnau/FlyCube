#include "Resource/VKResource.h"
#include <View/VKView.h>
#include <Device/DXDevice.h>

VKResource::VKResource(VKDevice& device)
    : m_device(device)
{
}

gli::format VKResource::GetFormat() const
{
    return m_format;
}

void VKResource::SetName(const std::string& name)
{
}
