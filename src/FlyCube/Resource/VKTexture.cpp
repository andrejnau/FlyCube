#include "Resource/VKTexture.h"

#include "Device/VKDevice.h"
#include "Memory/VKMemory.h"

VKTexture::VKTexture(PassKey<VKTexture> pass_key, VKDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<VKTexture> VKTexture::WrapSwapchainImage(VKDevice& device, vk::Image image, const TextureDesc& desc)
{
    std::shared_ptr<VKTexture> self = std::make_shared<VKTexture>(PassKey<VKTexture>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->is_back_buffer_ = true;
    self->image_ = image;
    self->image_desc_ = desc;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<VKTexture> VKTexture::CreateImage(VKDevice& device, const TextureDesc& desc)
{
    vk::ImageUsageFlags usage = {};
    if (desc.usage & BindFlag::kDepthStencil) {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
    }
    if (desc.usage & BindFlag::kShaderResource) {
        usage |= vk::ImageUsageFlagBits::eSampled;
    }
    if (desc.usage & BindFlag::kRenderTarget) {
        usage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    }
    if (desc.usage & BindFlag::kUnorderedAccess) {
        usage |= vk::ImageUsageFlagBits::eStorage;
    }
    if (desc.usage & BindFlag::kCopyDest) {
        usage |= vk::ImageUsageFlagBits::eTransferDst;
    }
    if (desc.usage & BindFlag::kCopySource) {
        usage |= vk::ImageUsageFlagBits::eTransferSrc;
    }
    if (desc.usage & BindFlag::kShadingRateSource) {
        usage |= vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR;
    }

    vk::ImageCreateInfo image_info = {};
    switch (desc.type) {
    case TextureType::k1D:
        image_info.imageType = vk::ImageType::e1D;
        break;
    case TextureType::k2D:
        image_info.imageType = vk::ImageType::e2D;
        break;
    case TextureType::k3D:
        image_info.imageType = vk::ImageType::e3D;
        break;
    }
    image_info.extent.width = desc.width;
    image_info.extent.height = desc.height;
    if (desc.type == TextureType::k3D) {
        image_info.extent.depth = desc.depth_or_array_layers;
    } else {
        image_info.extent.depth = 1;
    }
    image_info.mipLevels = desc.mip_levels;
    if (desc.type == TextureType::k3D) {
        image_info.arrayLayers = 1;
    } else {
        image_info.arrayLayers = desc.depth_or_array_layers;
    }
    image_info.format = static_cast<vk::Format>(desc.format);
    image_info.tiling = vk::ImageTiling::eOptimal;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage = usage;
    image_info.samples = static_cast<vk::SampleCountFlagBits>(desc.sample_count);
    image_info.sharingMode = vk::SharingMode::eExclusive;

    if (image_info.arrayLayers % 6 == 0) {
        image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    }

    std::shared_ptr<VKTexture> self = std::make_shared<VKTexture>(PassKey<VKTexture>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->image_owned_ = device.GetDevice().createImageUnique(image_info);
    self->image_ = self->image_owned_.get();
    self->image_desc_ = desc;
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

void VKTexture::CommitMemory(MemoryType memory_type)
{
    MemoryRequirements mem_requirements = GetMemoryRequirements();
    vk::MemoryDedicatedAllocateInfo dedicated_allocate_info = {};
    dedicated_allocate_info.image = GetImage();
    commited_memory_ = std::make_shared<VKMemory>(device_, mem_requirements.size, memory_type,
                                                  mem_requirements.memory_type_bits, &dedicated_allocate_info);
    BindMemory(commited_memory_, 0);
}

void VKTexture::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    vk::DeviceMemory vk_memory = memory->As<VKMemory>().GetMemory();
    device_.GetDevice().bindImageMemory(GetImage(), vk_memory, offset);
}

MemoryRequirements VKTexture::GetMemoryRequirements() const
{
    vk::MemoryRequirements2 mem_requirements = {};
    vk::ImageMemoryRequirementsInfo2KHR image_mem_req = {};
    image_mem_req.image = GetImage();
    device_.GetDevice().getImageMemoryRequirements2(&image_mem_req, &mem_requirements);
    return { mem_requirements.memoryRequirements.size, mem_requirements.memoryRequirements.alignment,
             mem_requirements.memoryRequirements.memoryTypeBits };
}

uint64_t VKTexture::GetWidth() const
{
    return image_desc_.width;
}

uint32_t VKTexture::GetHeight() const
{
    return image_desc_.height;
}

uint16_t VKTexture::GetLayerCount() const
{
    return image_desc_.depth_or_array_layers;
}

uint16_t VKTexture::GetLevelCount() const
{
    return image_desc_.mip_levels;
}

uint32_t VKTexture::GetSampleCount() const
{
    return image_desc_.sample_count;
}

void VKTexture::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    info.objectType = GetImage().objectType;
    info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkImage>(GetImage()));
    device_.GetDevice().setDebugUtilsObjectNameEXT(info);
}

vk::Image VKTexture::GetImage() const
{
    return image_;
}
