#include "View/MTView.h"
#include <Device/MTDevice.h>
#include <Resource/MTResource.h>

MTView::MTView(MTDevice& device, const std::shared_ptr<MTResource>& resource, const ViewDesc& m_view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(m_view_desc)
{
}

std::shared_ptr<Resource> MTView::GetResource()
{
    return m_resource;
}

uint32_t MTView::GetDescriptorId() const
{
    return -1;
}

uint32_t MTView::GetBaseMipLevel() const
{
    return m_view_desc.base_mip_level;
}

uint32_t MTView::GetLevelCount() const
{
    return std::min<uint32_t>(m_view_desc.level_count, m_resource->GetLevelCount() - m_view_desc.base_mip_level);
}

uint32_t MTView::GetBaseArrayLayer() const
{
    return m_view_desc.base_array_layer;
}

uint32_t MTView::GetLayerCount() const
{
    return std::min<uint32_t>(m_view_desc.layer_count, m_resource->GetLayerCount() - m_view_desc.base_array_layer);
}
