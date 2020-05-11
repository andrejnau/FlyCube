#include "Resource/VKResource.h"
#include <View/VKView.h>
#include <Device/DXDevice.h>

VKResource::VKResource(VKDevice& device)
    : m_device(device)
{
}

void VKResource::SetName(const std::string& name)
{
}

std::shared_ptr<View> VKResource::CreateView(const ViewDesc& view_desc)
{
    return std::make_unique<VKView>(*this, view_desc);
}
