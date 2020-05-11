#include "View/VKView.h"
#include <Device/VKDevice.h>

VKView::VKView(VKDevice& device, const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc)
    : m_resource(resource)
{
}

std::shared_ptr<Resource> VKView::GetResource()
{
    return m_resource;
}
