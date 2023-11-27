#include "View/SWView.h"

#include "BindingSetLayout/SWBindingSetLayout.h"
#include "Device/SWDevice.h"
#include "Resource/SWResource.h"

SWView::SWView(SWDevice& device, const std::shared_ptr<SWResource>& resource, const ViewDesc& view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(view_desc)
{
}

std::shared_ptr<Resource> SWView::GetResource()
{
    return m_resource;
}

uint32_t SWView::GetDescriptorId() const
{
    return -1;
}

uint32_t SWView::GetBaseMipLevel() const
{
    return m_view_desc.base_mip_level;
}

uint32_t SWView::GetLevelCount() const
{
    return std::min<uint32_t>(m_view_desc.level_count, m_resource->GetLevelCount() - m_view_desc.base_mip_level);
}

uint32_t SWView::GetBaseArrayLayer() const
{
    return m_view_desc.base_array_layer;
}

uint32_t SWView::GetLayerCount() const
{
    return std::min<uint32_t>(m_view_desc.layer_count, m_resource->GetLayerCount() - m_view_desc.base_array_layer);
}
