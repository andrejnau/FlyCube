#include "Resource/VKResource.h"

#include "Device/VKDevice.h"
#include "Memory/VKMemory.h"
#include "Utilities/VKUtility.h"
#include "View/VKView.h"

namespace {

vk::AccelerationStructureTypeKHR Convert(AccelerationStructureType type)
{
    switch (type) {
    case AccelerationStructureType::kTopLevel:
        return vk::AccelerationStructureTypeKHR::eTopLevel;
    case AccelerationStructureType::kBottomLevel:
        return vk::AccelerationStructureTypeKHR::eBottomLevel;
    }
    assert(false);
    return {};
}

} // namespace

VKResource::VKResource(PassKey<VKResource> pass_key, VKDevice& device)
    : m_device(device)
{
}

// static
std::shared_ptr<VKResource> VKResource::WrapSwapchainImage(VKDevice& device,
                                                           vk::Image image,
                                                           gli::format format,
                                                           uint32_t width,
                                                           uint32_t height)
{
    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->m_format = format;
    self->m_is_back_buffer = true;
    self->m_image = {
        .res = image,
        .size = vk::Extent2D(width, height),
    };
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateImage(VKDevice& device,
                                                    TextureType type,
                                                    uint32_t bind_flag,
                                                    gli::format format,
                                                    uint32_t sample_count,
                                                    int width,
                                                    int height,
                                                    int depth,
                                                    int mip_levels)
{
    vk::ImageUsageFlags usage = {};
    if (bind_flag & BindFlag::kDepthStencil) {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
    }
    if (bind_flag & BindFlag::kShaderResource) {
        usage |= vk::ImageUsageFlagBits::eSampled;
    }
    if (bind_flag & BindFlag::kRenderTarget) {
        usage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    }
    if (bind_flag & BindFlag::kUnorderedAccess) {
        usage |= vk::ImageUsageFlagBits::eStorage;
    }
    if (bind_flag & BindFlag::kCopyDest) {
        usage |= vk::ImageUsageFlagBits::eTransferDst;
    }
    if (bind_flag & BindFlag::kCopySource) {
        usage |= vk::ImageUsageFlagBits::eTransferSrc;
    }
    if (bind_flag & BindFlag::kShadingRateSource) {
        usage |= vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR;
    }

    vk::ImageCreateInfo image_info = {};
    switch (type) {
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
    image_info.extent.width = width;
    image_info.extent.height = height;
    if (type == TextureType::k3D) {
        image_info.extent.depth = depth;
    } else {
        image_info.extent.depth = 1;
    }
    image_info.mipLevels = mip_levels;
    if (type == TextureType::k3D) {
        image_info.arrayLayers = 1;
    } else {
        image_info.arrayLayers = depth;
    }
    image_info.format = static_cast<vk::Format>(format);
    image_info.tiling = vk::ImageTiling::eOptimal;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage = usage;
    image_info.samples = static_cast<vk::SampleCountFlagBits>(sample_count);
    image_info.sharingMode = vk::SharingMode::eExclusive;

    if (image_info.arrayLayers % 6 == 0) {
        image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    }

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->m_format = format;
    self->m_image_owned = device.GetDevice().createImageUnique(image_info);
    self->m_image = {
        .res = self->m_image_owned.get(),
        .size = vk::Extent2D(width, height),
        .level_count = static_cast<uint32_t>(mip_levels),
        .sample_count = sample_count,
        .array_layers = static_cast<uint32_t>(depth),
    };
    self->SetInitialState(ResourceState::kUndefined);
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateBuffer(VKDevice& device, uint32_t bind_flag, uint32_t buffer_size)
{
    if (buffer_size == 0) {
        return nullptr;
    }

    vk::BufferCreateInfo buffer_info = {};
    buffer_info.size = buffer_size;
    buffer_info.usage = vk::BufferUsageFlagBits::eShaderDeviceAddress;

    if (bind_flag & BindFlag::kVertexBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
    }
    if (bind_flag & BindFlag::kIndexBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
    }
    if (bind_flag & BindFlag::kConstantBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
    }
    if (bind_flag & BindFlag::kUnorderedAccess) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
    }
    if (bind_flag & BindFlag::kShaderResource) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        buffer_info.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
    }
    if (bind_flag & BindFlag::kAccelerationStructure) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
    }
    if (bind_flag & BindFlag::kCopySource) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferSrc;
    }
    if (bind_flag & BindFlag::kCopyDest) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eTransferDst;
    }
    if (bind_flag & BindFlag::kShaderTable) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
    }
    if (bind_flag & BindFlag::kIndirectBuffer) {
        buffer_info.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
    }

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->m_resource_type = ResourceType::kBuffer;
    self->m_buffer = {
        .res = device.GetDevice().createBufferUnique(buffer_info),
        .size = buffer_size,
    };
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateSampler(VKDevice& device, const SamplerDesc& desc)
{
    vk::SamplerCreateInfo sampler_info = {};
    sampler_info.magFilter = vk::Filter::eLinear;
    sampler_info.minFilter = vk::Filter::eLinear;
    sampler_info.anisotropyEnable = true;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = std::numeric_limits<float>::max();

    /*switch (desc.filter)
    {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }*/

    switch (desc.mode) {
    case SamplerTextureAddressMode::kWrap:
        sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        break;
    }

    switch (desc.func) {
    case SamplerComparisonFunc::kNever:
        sampler_info.compareEnable = false;
        sampler_info.compareOp = vk::CompareOp::eNever;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_info.compareEnable = true;
        sampler_info.compareOp = vk::CompareOp::eAlways;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_info.compareEnable = true;
        sampler_info.compareOp = vk::CompareOp::eLess;
        break;
    }

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->m_resource_type = ResourceType::kSampler;
    self->m_sampler = {
        .res = device.GetDevice().createSamplerUnique(sampler_info),
    };
    return self;
}

// static
std::shared_ptr<VKResource> VKResource::CreateAccelerationStructure(
    VKDevice& device,
    AccelerationStructureType type,
    const std::shared_ptr<Resource>& acceleration_structures_memory,
    uint64_t offset)
{
    vk::AccelerationStructureCreateInfoKHR acceleration_structure_create_info = {};
    acceleration_structure_create_info.buffer = acceleration_structures_memory->As<VKResource>().GetBuffer();
    acceleration_structure_create_info.offset = offset;
    acceleration_structure_create_info.size = 0;
    acceleration_structure_create_info.type = Convert(type);

    std::shared_ptr<VKResource> self = std::make_shared<VKResource>(PassKey<VKResource>(), device);
    self->m_resource_type = ResourceType::kAccelerationStructure;
    self->m_acceleration_structure =
        device.GetDevice().createAccelerationStructureKHRUnique(acceleration_structure_create_info);
    return self;
}

void VKResource::CommitMemory(MemoryType memory_type)
{
    MemoryRequirements mem_requirements = GetMemoryRequirements();
    vk::MemoryDedicatedAllocateInfoKHR dedicated_allocate_info = {};
    vk::MemoryDedicatedAllocateInfoKHR* p_dedicated_allocate_info = nullptr;
    if (m_resource_type == ResourceType::kBuffer) {
        dedicated_allocate_info.buffer = GetBuffer();
        p_dedicated_allocate_info = &dedicated_allocate_info;
    } else if (m_resource_type == ResourceType::kTexture) {
        dedicated_allocate_info.image = GetImage();
        p_dedicated_allocate_info = &dedicated_allocate_info;
    }
    m_commited_memory = std::make_shared<VKMemory>(m_device, mem_requirements.size, memory_type,
                                                   mem_requirements.memory_type_bits, p_dedicated_allocate_info);
    BindMemory(m_commited_memory, 0);
}

void VKResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory_type = memory->GetMemoryType();
    m_vk_memory = memory->As<VKMemory>().GetMemory();

    if (m_resource_type == ResourceType::kBuffer) {
        m_device.GetDevice().bindBufferMemory(GetBuffer(), m_vk_memory, offset);
    } else if (m_resource_type == ResourceType::kTexture) {
        m_device.GetDevice().bindImageMemory(GetImage(), m_vk_memory, offset);
    }
}

uint64_t VKResource::GetWidth() const
{
    if (m_resource_type == ResourceType::kTexture) {
        return m_image.size.width;
    }
    return m_buffer.size;
}

uint32_t VKResource::GetHeight() const
{
    return m_image.size.height;
}

uint16_t VKResource::GetLayerCount() const
{
    return m_image.array_layers;
}

uint16_t VKResource::GetLevelCount() const
{
    return m_image.level_count;
}

uint32_t VKResource::GetSampleCount() const
{
    return m_image.sample_count;
}

uint64_t VKResource::GetAccelerationStructureHandle() const
{
    return m_device.GetDevice().getAccelerationStructureAddressKHR({ GetAccelerationStructure() });
}

void VKResource::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    if (m_resource_type == ResourceType::kBuffer) {
        info.objectType = GetBuffer().objectType;
        info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkBuffer>(GetBuffer()));
    } else if (m_resource_type == ResourceType::kTexture) {
        info.objectType = GetImage().objectType;
        info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkImage>(GetImage()));
    }
    m_device.GetDevice().setDebugUtilsObjectNameEXT(info);
}

uint8_t* VKResource::Map()
{
    uint8_t* dst_data = nullptr;
    std::ignore =
        m_device.GetDevice().mapMemory(m_vk_memory, 0, VK_WHOLE_SIZE, {}, reinterpret_cast<void**>(&dst_data));
    return dst_data;
}

void VKResource::Unmap()
{
    m_device.GetDevice().unmapMemory(m_vk_memory);
}

MemoryRequirements VKResource::GetMemoryRequirements() const
{
    vk::MemoryRequirements2 mem_requirements = {};
    if (m_resource_type == ResourceType::kBuffer) {
        vk::BufferMemoryRequirementsInfo2KHR buffer_mem_req = {};
        buffer_mem_req.buffer = GetBuffer();
        m_device.GetDevice().getBufferMemoryRequirements2(&buffer_mem_req, &mem_requirements);
    } else if (m_resource_type == ResourceType::kTexture) {
        vk::ImageMemoryRequirementsInfo2KHR image_mem_req = {};
        image_mem_req.image = GetImage();
        m_device.GetDevice().getImageMemoryRequirements2(&image_mem_req, &mem_requirements);
    }
    return { mem_requirements.memoryRequirements.size, mem_requirements.memoryRequirements.alignment,
             mem_requirements.memoryRequirements.memoryTypeBits };
}

const vk::Image& VKResource::GetImage() const
{
    return m_image.res;
}

const vk::Buffer& VKResource::GetBuffer() const
{
    return m_buffer.res.get();
}

const vk::Sampler& VKResource::GetSampler() const
{
    return m_sampler.res.get();
}

const vk::AccelerationStructureKHR& VKResource::GetAccelerationStructure() const
{
    return m_acceleration_structure.get();
}
