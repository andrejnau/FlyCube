#include "View/VKView.h"

#include "BindingSetLayout/VKBindingSetLayout.h"
#include "BindlessTypedViewPool/VKBindlessTypedViewPool.h"
#include "Device/VKDevice.h"
#include "Resource/VKResource.h"
#include "Utilities/NotReached.h"

namespace {

vk::ImageViewType GetImageViewType(ViewDimension dimension)
{
    switch (dimension) {
    case ViewDimension::kTexture1D:
        return vk::ImageViewType::e1D;
    case ViewDimension::kTexture1DArray:
        return vk::ImageViewType::e1DArray;
    case ViewDimension::kTexture2D:
    case ViewDimension::kTexture2DMS:
        return vk::ImageViewType::e2D;
    case ViewDimension::kTexture2DArray:
    case ViewDimension::kTexture2DMSArray:
        return vk::ImageViewType::e2DArray;
    case ViewDimension::kTexture3D:
        return vk::ImageViewType::e3D;
    case ViewDimension::kTextureCube:
        return vk::ImageViewType::eCube;
    case ViewDimension::kTextureCubeArray:
        return vk::ImageViewType::eCubeArray;
    default:
        NOTREACHED();
    }
}

} // namespace

VKView::VKView(VKDevice& device, const std::shared_ptr<VKResource>& resource, const ViewDesc& view_desc)
    : device_(device)
    , resource_(resource)
    , view_desc_(view_desc)
{
    if (resource) {
        CreateView();
    }

    if (view_desc_.bindless) {
        bindless_view_pool_ = std::make_shared<VKBindlessTypedViewPool>(device_, view_desc_.view_type, 1);
        bindless_view_pool_->WriteViewImpl(0, *this);
    }
}

void VKView::CreateView()
{
    switch (view_desc_.view_type) {
    case ViewType::kSampler:
        descriptor_image_.sampler = resource_->GetSampler();
        descriptor_.pImageInfo = &descriptor_image_;
        break;
    case ViewType::kTexture: {
        CreateImageView();
        descriptor_image_.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        descriptor_image_.imageView = image_view_.get();
        descriptor_.pImageInfo = &descriptor_image_;
        break;
    }
    case ViewType::kRWTexture: {
        CreateImageView();
        descriptor_image_.imageLayout = vk::ImageLayout::eGeneral;
        descriptor_image_.imageView = image_view_.get();
        descriptor_.pImageInfo = &descriptor_image_;
        break;
    }
    case ViewType::kAccelerationStructure: {
        acceleration_structure_ = resource_->GetAccelerationStructure();
        descriptor_acceleration_structure_.accelerationStructureCount = 1;
        descriptor_acceleration_structure_.pAccelerationStructures = &acceleration_structure_;
        descriptor_.pNext = &descriptor_acceleration_structure_;
        break;
    }
    case ViewType::kShadingRateSource:
    case ViewType::kRenderTarget:
    case ViewType::kDepthStencil: {
        CreateImageView();
        break;
    }
    case ViewType::kConstantBuffer:
    case ViewType::kStructuredBuffer:
    case ViewType::kRWStructuredBuffer:
    case ViewType::kByteAddressBuffer:
    case ViewType::kRWByteAddressBuffer: {
        uint64_t size = std::min(resource_->GetWidth() - view_desc_.offset, view_desc_.buffer_size);
        descriptor_buffer_.buffer = resource_->GetBuffer();
        descriptor_buffer_.offset = view_desc_.offset;
        descriptor_buffer_.range = size;
        descriptor_.pBufferInfo = &descriptor_buffer_;
        break;
    }
    case ViewType::kBuffer:
    case ViewType::kRWBuffer:
        CreateBufferView();
        descriptor_.pTexelBufferView = &buffer_view_.get();
        break;
    default:
        NOTREACHED();
    }
}

void VKView::CreateImageView()
{
    vk::ImageViewCreateInfo image_view_desc = {};
    image_view_desc.image = resource_->GetImage();
    image_view_desc.format = static_cast<vk::Format>(resource_->GetFormat());
    image_view_desc.viewType = GetImageViewType(view_desc_.dimension);
    image_view_desc.subresourceRange.baseMipLevel = GetBaseMipLevel();
    image_view_desc.subresourceRange.levelCount = GetLevelCount();
    image_view_desc.subresourceRange.baseArrayLayer = GetBaseArrayLayer();
    image_view_desc.subresourceRange.layerCount = GetLayerCount();
    image_view_desc.subresourceRange.aspectMask = device_.GetAspectFlags(image_view_desc.format);

    if (image_view_desc.subresourceRange.aspectMask &
        (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) {
        if (view_desc_.plane_slice == 0) {
            image_view_desc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        } else {
            image_view_desc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eStencil;
            image_view_desc.components.g = vk::ComponentSwizzle::eR;
        }
    }

    image_view_ = device_.GetDevice().createImageViewUnique(image_view_desc);
}

void VKView::CreateBufferView()
{
    uint64_t size = std::min(resource_->GetWidth() - view_desc_.offset, view_desc_.buffer_size);
    vk::BufferViewCreateInfo buffer_view_desc = {};
    buffer_view_desc.buffer = resource_->GetBuffer();
    buffer_view_desc.format = static_cast<vk::Format>(view_desc_.buffer_format);
    buffer_view_desc.offset = view_desc_.offset;
    buffer_view_desc.range = size;
    buffer_view_ = device_.GetDevice().createBufferViewUnique(buffer_view_desc);
}

std::shared_ptr<Resource> VKView::GetResource()
{
    return resource_;
}

uint32_t VKView::GetDescriptorId() const
{
    if (bindless_view_pool_) {
        return bindless_view_pool_->GetBaseDescriptorId();
    }
    NOTREACHED();
}

uint32_t VKView::GetBaseMipLevel() const
{
    return view_desc_.base_mip_level;
}

uint32_t VKView::GetLevelCount() const
{
    return std::min<uint32_t>(view_desc_.level_count, resource_->GetLevelCount() - view_desc_.base_mip_level);
}

uint32_t VKView::GetBaseArrayLayer() const
{
    return view_desc_.base_array_layer;
}

uint32_t VKView::GetLayerCount() const
{
    return std::min<uint32_t>(view_desc_.layer_count, resource_->GetLayerCount() - view_desc_.base_array_layer);
}

vk::ImageView VKView::GetImageView() const
{
    return image_view_.get();
}

vk::WriteDescriptorSet VKView::GetDescriptor() const
{
    return descriptor_;
}
