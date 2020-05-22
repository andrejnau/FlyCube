#include "View/VKView.h"
#include <Device/VKDevice.h>
#include <Resource/VKResource.h>

VKView::VKView(VKDevice& device, const std::shared_ptr<VKResource>& resource, const ViewDesc& view_desc)
    : m_device(device)
    , m_resource(resource)
{
    if (!resource)
        return;
    decltype(auto) vk_resource = resource->As<VKResource>();

    switch (view_desc.res_type)
    {
    case ResourceType::kSrv:
    case ResourceType::kUav:
        CreateSrv(view_desc, vk_resource);
        break;
    case ResourceType::kRtv:
    case ResourceType::kDsv:
        CreateRTV(view_desc, vk_resource);
        break;
    }
}

void VKView::CreateSrv(const ViewDesc& view_desc, const VKResource& res)
{
    if (view_desc.dimension == ResourceDimension::kBuffer)
        return;
    m_view_info.image = res.image.res.get();
    m_view_info.format = res.image.format;
    m_view_info.subresourceRange.aspectMask = m_device.GetAspectFlags(m_view_info.format);
    m_view_info.subresourceRange.baseMipLevel = view_desc.level;
    if (view_desc.count == -1)
        m_view_info.subresourceRange.levelCount = res.image.level_count - m_view_info.subresourceRange.baseMipLevel;
    else
        m_view_info.subresourceRange.levelCount = view_desc.count;
    m_view_info.subresourceRange.baseArrayLayer = 0;
    m_view_info.subresourceRange.layerCount = res.image.array_layers;

    switch (view_desc.dimension)
    {
    case ResourceDimension::kTexture1D:
    {
        m_view_info.viewType = vk::ImageViewType::e1D;
        break;
    }
    case ResourceDimension::kTexture1DArray:
    {
        m_view_info.viewType = vk::ImageViewType::e1DArray;
        break;
    }
    case ResourceDimension::kTexture2D:
    {
        m_view_info.viewType = vk::ImageViewType::e2D;
        break;
    }
    case ResourceDimension::kTexture2DArray:
    {
        m_view_info.viewType = vk::ImageViewType::e2DArray;
        break;
    }
    case ResourceDimension::kTexture3D:
    {
        m_view_info.viewType = vk::ImageViewType::e3D;
        break;
    }
    case ResourceDimension::kTextureCube:
    {
        m_view_info.viewType = vk::ImageViewType::eCube;
        break;
    }
    case ResourceDimension::kTextureCubeArray:
    {
        m_view_info.viewType = vk::ImageViewType::eCubeArray;
        break;
    }
    default:
    {
        assert(false);
        break;
    }
    }

    m_srv = m_device.GetDevice().createImageViewUnique(m_view_info);
}

void VKView::CreateRTV(const ViewDesc& view_desc, const VKResource& res)
{
    m_view_info.image = res.image.res.get();
    m_view_info.format = res.image.format;
    if (res.image.array_layers > 1)
        m_view_info.viewType = vk::ImageViewType::e2DArray;
    else
        m_view_info.viewType = vk::ImageViewType::e2D;
    m_view_info.subresourceRange.aspectMask = m_device.GetAspectFlags(m_view_info.format);
    m_view_info.subresourceRange.baseMipLevel = view_desc.level;
    m_view_info.subresourceRange.levelCount = 1;
    m_view_info.subresourceRange.baseArrayLayer = 0;
    m_view_info.subresourceRange.layerCount = res.image.array_layers;
    m_om = m_device.GetDevice().createImageViewUnique(m_view_info);
}

std::shared_ptr<Resource> VKView::GetResource()
{
    return m_resource;
}

const vk::ImageViewCreateInfo& VKView::GeViewInfo() const
{
    return m_view_info;
}

vk::ImageView VKView::GetRtv() const
{
    return m_om.get();
}

vk::ImageView VKView::GetSrv() const
{
    return m_srv.get();
}
