#include "Resource/VKResource.h"

#include "Device/VKDevice.h"
#include "Memory/VKMemory.h"
#include "Utilities/NotReached.h"
#include "View/VKView.h"

namespace {

vk::AccelerationStructureTypeKHR Convert(AccelerationStructureType type)
{
    switch (type) {
    case AccelerationStructureType::kTopLevel:
        return vk::AccelerationStructureTypeKHR::eTopLevel;
    case AccelerationStructureType::kBottomLevel:
        return vk::AccelerationStructureTypeKHR::eBottomLevel;
    default:
        NOTREACHED();
    }
}

vk::Filter ConvertToFilter(SamplerFilter filter)
{
    switch (filter) {
    case SamplerFilter::kNearest:
        return vk::Filter::eNearest;
    case SamplerFilter::kLinear:
        return vk::Filter::eLinear;
    default:
        NOTREACHED();
    }
}

vk::SamplerMipmapMode ConvertToSamplerMipmapMode(SamplerFilter filter)
{
    switch (filter) {
    case SamplerFilter::kNearest:
        return vk::SamplerMipmapMode::eNearest;
    case SamplerFilter::kLinear:
        return vk::SamplerMipmapMode::eLinear;
    default:
        NOTREACHED();
    }
}

vk::SamplerAddressMode ConvertToSamplerAddressMode(SamplerAddressMode mode)
{
    switch (mode) {
    case SamplerAddressMode::kRepeat:
        return vk::SamplerAddressMode::eRepeat;
    case SamplerAddressMode::kMirrorRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case SamplerAddressMode::kClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    case SamplerAddressMode::kMirrorClampToEdge:
        return vk::SamplerAddressMode::eMirrorClampToEdge;
    case SamplerAddressMode::kClampToBorder:
        return vk::SamplerAddressMode::eClampToBorder;
    default:
        NOTREACHED();
    }
}

vk::BorderColor ConvertToBorderColor(SamplerBorderColor border_color)
{
    switch (border_color) {
    case SamplerBorderColor::kTransparentBlack:
        return vk::BorderColor::eFloatTransparentBlack;
    case SamplerBorderColor::kOpaqueBlack:
        return vk::BorderColor::eFloatOpaqueBlack;
    case SamplerBorderColor::kOpaqueWhite:
        return vk::BorderColor::eFloatOpaqueWhite;
    default:
        NOTREACHED();
    }
}

vk::SamplerReductionMode ConvertToSamplerReductionMode(SamplerReductionMode reduction_mode)
{
    switch (reduction_mode) {
    case SamplerReductionMode::kWeightedAverage:
        return vk::SamplerReductionMode::eWeightedAverage;
    case SamplerReductionMode::kMinimum:
        return vk::SamplerReductionMode::eMin;
    case SamplerReductionMode::kMaximum:
        return vk::SamplerReductionMode::eMax;
    default:
        NOTREACHED();
    }
}

} // namespace

VKResource::VKResource(PassKey<VKResource> pass_key, VKDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<VKResource> VKResource::WrapSwapchainImage(VKDevice& device,
                                                           vk::Image image,
                                                           const TextureDesc& desc,
                                                           vk::ImageUsageFlags usage)
{
    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->is_back_buffer_ = true;
    self->image_ = {
        .res = image,
        .desc = desc,
        .usage = usage,
    };
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateImage(VKDevice& device, const TextureDesc& desc)
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

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->image_owned_ = device.GetDevice().createImageUnique(image_info);
    self->image_ = {
        .res = self->image_owned_.get(),
        .desc = desc,
        .flags = image_info.flags,
        .usage = image_info.usage,
    };
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateBuffer(VKDevice& device, const BufferDesc& desc)
{
    if (desc.size == 0) {
        return nullptr;
    }

    vk::BufferCreateInfo buffer_info = {};
    buffer_info.size = desc.size;
    buffer_info.usage = vk::BufferUsageFlagBits::eShaderDeviceAddress;

    if (desc.usage & BindFlag::kVertexBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
    }
    if (desc.usage & BindFlag::kIndexBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
    }
    if (desc.usage & BindFlag::kConstantBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
    }
    if (desc.usage & BindFlag::kUnorderedAccess) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
    }
    if (desc.usage & BindFlag::kShaderResource) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
    }
    if (desc.usage & BindFlag::kAccelerationStructure) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
    }
    if (desc.usage & BindFlag::kCopySource) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferSrc;
    }
    if (desc.usage & BindFlag::kCopyDest) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    }
    if (desc.usage & BindFlag::kShaderTable) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
    }
    if (desc.usage & BindFlag::kIndirectBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
    }

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->resource_type_ = ResourceType::kBuffer;
    self->buffer_ = {
        .res = device.GetDevice().createBufferUnique(buffer_info),
        .size = desc.size,
    };
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateSampler(VKDevice& device, const SamplerDesc& desc)
{
    vk::SamplerCreateInfo sampler_info = {};
    sampler_info.magFilter = ConvertToFilter(desc.mag_filter);
    sampler_info.minFilter = ConvertToFilter(desc.min_filter);
    sampler_info.mipmapMode = ConvertToSamplerMipmapMode(desc.mip_filter);
    sampler_info.addressModeU = ConvertToSamplerAddressMode(desc.address_mode_u);
    sampler_info.addressModeV = ConvertToSamplerAddressMode(desc.address_mode_v);
    sampler_info.addressModeW = ConvertToSamplerAddressMode(desc.address_mode_w);
    sampler_info.mipLodBias = desc.mip_lod_bias;
    sampler_info.anisotropyEnable = desc.anisotropy_enable;
    sampler_info.maxAnisotropy = desc.max_anisotropy;
    sampler_info.compareEnable = desc.compare_enable;
    sampler_info.compareOp = ConvertToCompareOp(desc.compare_func);
    sampler_info.minLod = desc.min_lod;
    sampler_info.maxLod = desc.max_lod;
    sampler_info.borderColor = ConvertToBorderColor(desc.border_color);

    vk::SamplerReductionModeCreateInfo sampler_reduction_mode_info = {};
    sampler_reduction_mode_info.reductionMode = ConvertToSamplerReductionMode(desc.reduction_mode);
    if (sampler_reduction_mode_info.reductionMode != vk::SamplerReductionMode::eWeightedAverage) {
        sampler_info.pNext = &sampler_reduction_mode_info;
    }

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->resource_type_ = ResourceType::kSampler;
    self->sampler_ = {
        .res = device.GetDevice().createSamplerUnique(sampler_info),
    };
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateAccelerationStructure(VKDevice& device,
                                                                    const AccelerationStructureDesc& desc)
{
    vk::AccelerationStructureCreateInfoKHR acceleration_structure_create_info = {};
    acceleration_structure_create_info.buffer = desc.buffer->As<VKResource>().GetBuffer();
    acceleration_structure_create_info.offset = desc.buffer_offset;
    acceleration_structure_create_info.size = desc.size;
    acceleration_structure_create_info.type = Convert(desc.type);

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->resource_type_ = ResourceType::kAccelerationStructure;
    self->acceleration_structure_ =
        device.GetDevice().createAccelerationStructureKHRUnique(acceleration_structure_create_info);
    return self;
}

void VKResource::CommitMemory(MemoryType memory_type)
{
    MemoryRequirements mem_requirements = GetMemoryRequirements();
    vk::MemoryDedicatedAllocateInfoKHR dedicated_allocate_info = {};
    vk::MemoryDedicatedAllocateInfoKHR* p_dedicated_allocate_info = nullptr;
    if (resource_type_ == ResourceType::kBuffer) {
        dedicated_allocate_info.buffer = GetBuffer();
        p_dedicated_allocate_info = &dedicated_allocate_info;
    } else if (resource_type_ == ResourceType::kTexture) {
        dedicated_allocate_info.image = GetImage();
        p_dedicated_allocate_info = &dedicated_allocate_info;
    }
    commited_memory_ = std::make_shared<VKMemory>(device_, mem_requirements.size, memory_type,
                                                  mem_requirements.memory_type_bits, p_dedicated_allocate_info);
    BindMemory(commited_memory_, 0);
}

void VKResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    vk_memory_ = memory->As<VKMemory>().GetMemory();

    if (resource_type_ == ResourceType::kBuffer) {
        device_.GetDevice().bindBufferMemory(GetBuffer(), vk_memory_, offset);
    } else if (resource_type_ == ResourceType::kTexture) {
        device_.GetDevice().bindImageMemory(GetImage(), vk_memory_, offset);
    }
}

uint64_t VKResource::GetWidth() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return image_.desc.width;
    }
    assert(resource_type_ == ResourceType::kBuffer);
    return buffer_.size;
}

uint32_t VKResource::GetHeight() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return image_.desc.height;
    }
    return 1;
}

uint16_t VKResource::GetLayerCount() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return image_.desc.depth_or_array_layers;
    }
    return 1;
}

uint16_t VKResource::GetLevelCount() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return image_.desc.mip_levels;
    }
    return 1;
}

uint32_t VKResource::GetSampleCount() const
{
    if (resource_type_ == ResourceType::kTexture) {
        return image_.desc.sample_count;
    }
    return 1;
}

uint64_t VKResource::GetAccelerationStructureHandle() const
{
    return device_.GetDevice().getAccelerationStructureAddressKHR({ GetAccelerationStructure() });
}

void VKResource::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    if (resource_type_ == ResourceType::kBuffer) {
        info.objectType = GetBuffer().objectType;
        info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkBuffer>(GetBuffer()));
    } else if (resource_type_ == ResourceType::kTexture) {
        info.objectType = GetImage().objectType;
        info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkImage>(GetImage()));
    }
    device_.GetDevice().setDebugUtilsObjectNameEXT(info);
}

uint8_t* VKResource::Map()
{
    uint8_t* dst_data = nullptr;
    std::ignore = device_.GetDevice().mapMemory(vk_memory_, 0, VK_WHOLE_SIZE, {}, reinterpret_cast<void**>(&dst_data));
    return dst_data;
}

void VKResource::Unmap()
{
    device_.GetDevice().unmapMemory(vk_memory_);
}

MemoryRequirements VKResource::GetMemoryRequirements() const
{
    vk::MemoryRequirements2 mem_requirements = {};
    if (resource_type_ == ResourceType::kBuffer) {
        vk::BufferMemoryRequirementsInfo2KHR buffer_mem_req = {};
        buffer_mem_req.buffer = GetBuffer();
        device_.GetDevice().getBufferMemoryRequirements2(&buffer_mem_req, &mem_requirements);
    } else if (resource_type_ == ResourceType::kTexture) {
        vk::ImageMemoryRequirementsInfo2KHR image_mem_req = {};
        image_mem_req.image = GetImage();
        device_.GetDevice().getImageMemoryRequirements2(&image_mem_req, &mem_requirements);
    }
    return { mem_requirements.memoryRequirements.size, mem_requirements.memoryRequirements.alignment,
             mem_requirements.memoryRequirements.memoryTypeBits };
}

const vk::Image& VKResource::GetImage() const
{
    return image_.res;
}

const vk::Buffer& VKResource::GetBuffer() const
{
    return buffer_.res.get();
}

const vk::Sampler& VKResource::GetSampler() const
{
    return sampler_.res.get();
}

const vk::AccelerationStructureKHR& VKResource::GetAccelerationStructure() const
{
    return acceleration_structure_.get();
}

vk::ImageCreateFlags VKResource::GetImageCreateFlags() const
{
    return image_.flags;
}

vk::ImageUsageFlags VKResource::GetImageUsageFlags() const
{
    return image_.usage;
}
