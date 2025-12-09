#include "Resource/VKSampler.h"

#include "Device/VKDevice.h"
#include "Utilities/NotReached.h"

namespace {

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

VKSampler::VKSampler(PassKey<VKSampler> pass_key, VKDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<VKSampler> VKSampler::CreateSampler(VKDevice& device, const SamplerDesc& desc)
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

    std::shared_ptr<VKSampler> self = std::make_shared<VKSampler>(PassKey<VKSampler>(), device);
    self->resource_type_ = ResourceType::kSampler;
    self->sampler_ = device.GetDevice().createSamplerUnique(sampler_info);
    return self;
}

void VKSampler::SetName(const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT info = {};
    info.pObjectName = name.c_str();
    info.objectType = GetSampler().objectType;
    info.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkSampler>(GetSampler()));
    device_.GetDevice().setDebugUtilsObjectNameEXT(info);
}

vk::Sampler VKSampler::GetSampler() const
{
    return sampler_.get();
}
