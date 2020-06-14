#include "View/VKView.h"
#include <Device/VKDevice.h>
#include <Resource/VKResource.h>

VKView::VKView(VKDevice& device, const std::shared_ptr<VKResource>& resource, const ViewDesc& view_desc)
    : m_device(device)
    , m_resource(resource)
    , m_view_desc(view_desc)
{
    if (!resource)
        return;
    decltype(auto) vk_resource = resource->As<VKResource>();

    switch (view_desc.view_type)
    {
    case ViewType::kShaderResource:
    case ViewType::kUnorderedAccess:
        CreateSrv(view_desc, vk_resource);
        break;
    case ViewType::kRenderTarget:
    case ViewType::kDepthStencil:
        CreateRTV(view_desc, vk_resource);
        break;
    }

    if (view_desc.bindless)
    {
        vk::DescriptorType type = {};
        if (view_desc.dimension == ResourceDimension::kBuffer)
            type = vk::DescriptorType::eStorageBuffer;
        else
            type = vk::DescriptorType::eSampledImage;
        auto& pool = device.GetGPUBindlessDescriptorPool(type);
        m_range = std::make_shared<VKGPUDescriptorPoolRange>(pool.Allocate(1));

        std::list<vk::DescriptorImageInfo> list_image_info;
        std::list<vk::DescriptorBufferInfo> list_buffer_info;
        std::list<vk::WriteDescriptorSetAccelerationStructureNV> list_as;

        vk::WriteDescriptorSet descriptor_write = {};
        descriptor_write.dstSet = m_range->GetDescriptoSet();
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = m_range->GetOffset();
        descriptor_write.descriptorType = type;
        descriptor_write.descriptorCount = 1;

        WriteView(descriptor_write, list_image_info, list_buffer_info, list_as);

        if (descriptor_write.pImageInfo || descriptor_write.pBufferInfo || descriptor_write.pNext)
            m_device.GetDevice().updateDescriptorSets(1, &descriptor_write, 0, nullptr);
    }
}

void VKView::WriteView(vk::WriteDescriptorSet& descriptor_write,
                       std::list<vk::DescriptorImageInfo>& list_image_info,
                       std::list<vk::DescriptorBufferInfo>& list_buffer_info,
                       std::list<vk::WriteDescriptorSetAccelerationStructureNV>& list_as)
{
    if (!m_resource)
        return;

    switch (descriptor_write.descriptorType)
    {
    case vk::DescriptorType::eSampler:
    {
        list_image_info.emplace_back();
        vk::DescriptorImageInfo& image_info = list_image_info.back();
        image_info.sampler = m_resource->sampler.res.get();
        descriptor_write.pImageInfo = &image_info;
        break;
    }
    case vk::DescriptorType::eSampledImage:
    {
        list_image_info.emplace_back();
        vk::DescriptorImageInfo& image_info = list_image_info.back();
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = GetSrv();
        if (image_info.imageView)
            descriptor_write.pImageInfo = &image_info;
        break;
    }
    case vk::DescriptorType::eStorageImage:
    {
        list_image_info.emplace_back();
        vk::DescriptorImageInfo& image_info = list_image_info.back();
        image_info.imageLayout = vk::ImageLayout::eGeneral;
        image_info.imageView = GetSrv();
        if (image_info.imageView)
            descriptor_write.pImageInfo = &image_info;
        break;
    }
    case vk::DescriptorType::eUniformTexelBuffer:
    case vk::DescriptorType::eStorageTexelBuffer:
    case vk::DescriptorType::eUniformBuffer:
    case vk::DescriptorType::eStorageBuffer:
    {
        list_buffer_info.emplace_back();
        vk::DescriptorBufferInfo& buffer_info = list_buffer_info.back();
        buffer_info.buffer = m_resource->buffer.res.get();
        buffer_info.offset = m_view_desc.offset;
        buffer_info.range = VK_WHOLE_SIZE;
        descriptor_write.pBufferInfo = &buffer_info;
        break;
    }
    case vk::DescriptorType::eAccelerationStructureNV:
    {
        list_as.emplace_back();
        vk::WriteDescriptorSetAccelerationStructureNV& descriptorAccelerationStructureInfo = list_as.back();
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = &m_resource->as.acceleration_structure.get();
        descriptor_write.pNext = &descriptorAccelerationStructureInfo;
        break;
    }
    case vk::DescriptorType::eUniformBufferDynamic:
    case vk::DescriptorType::eStorageBufferDynamic:
    default:
        assert(false);
        break;
    }
}

void VKView::CreateSrv(const ViewDesc& view_desc, const VKResource& res)
{
    if (view_desc.dimension == ResourceDimension::kBuffer || view_desc.dimension == ResourceDimension::kRaytracingAccelerationStructure)
        return;
    m_view_info.image = res.image.res;
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
    m_view_info.image = res.image.res;
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

uint32_t VKView::GetDescriptorId() const
{
    if (m_range)
        return m_range->GetOffset();
    return -1;
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

uint32_t VKView::GetBaseMipLevel() const
{
    return m_view_info.subresourceRange.baseMipLevel;
}

uint32_t VKView::GetLevelCount() const
{
    return m_view_info.subresourceRange.levelCount;
}

uint32_t VKView::GetBaseArrayLayer() const
{
    return m_view_info.subresourceRange.baseArrayLayer;
}

uint32_t VKView::GetLayerCount() const
{
    return m_view_info.subresourceRange.layerCount;
}
