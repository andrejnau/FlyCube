#include "Resource/MTSampler.h"

#include "Device/MTDevice.h"
#include "Utilities/Logging.h"
#include "Utilities/NotReached.h"

namespace {

MTLSamplerMinMagFilter ConvertToSamplerMinMagFilter(SamplerFilter filter)
{
    switch (filter) {
    case SamplerFilter::kNearest:
        return MTLSamplerMinMagFilterNearest;
    case SamplerFilter::kLinear:
        return MTLSamplerMinMagFilterLinear;
    default:
        NOTREACHED();
    }
}

MTLSamplerMipFilter ConvertToSamplerMipFilter(SamplerFilter filter)
{
    switch (filter) {
    case SamplerFilter::kNearest:
        return MTLSamplerMipFilterNearest;
    case SamplerFilter::kLinear:
        return MTLSamplerMipFilterLinear;
    default:
        NOTREACHED();
    }
}

MTLSamplerAddressMode ConvertToSamplerAddressMode(SamplerAddressMode mode)
{
    switch (mode) {
    case SamplerAddressMode::kRepeat:
        return MTLSamplerAddressModeRepeat;
    case SamplerAddressMode::kMirrorRepeat:
        return MTLSamplerAddressModeMirrorRepeat;
    case SamplerAddressMode::kClampToEdge:
        return MTLSamplerAddressModeClampToEdge;
    case SamplerAddressMode::kMirrorClampToEdge:
        return MTLSamplerAddressModeMirrorClampToEdge;
    case SamplerAddressMode::kClampToBorder:
        return MTLSamplerAddressModeClampToBorderColor;
    default:
        NOTREACHED();
    }
}

MTLSamplerBorderColor ConvertToSamplerBorderColor(SamplerBorderColor border_color)
{
    switch (border_color) {
    case SamplerBorderColor::kTransparentBlack:
        return MTLSamplerBorderColorTransparentBlack;
    case SamplerBorderColor::kOpaqueBlack:
        return MTLSamplerBorderColorOpaqueBlack;
    case SamplerBorderColor::kOpaqueWhite:
        return MTLSamplerBorderColorOpaqueWhite;
    default:
        NOTREACHED();
    }
}

MTLSamplerReductionMode ConvertToSamplerReductionMode(SamplerReductionMode reduction_mode)
{
    switch (reduction_mode) {
    case SamplerReductionMode::kWeightedAverage:
        return MTLSamplerReductionModeWeightedAverage;
    case SamplerReductionMode::kMinimum:
        return MTLSamplerReductionModeMinimum;
    case SamplerReductionMode::kMaximum:
        return MTLSamplerReductionModeMaximum;
    default:
        NOTREACHED();
    }
}

} // namespace

MTSampler::MTSampler(PassKey<MTSampler> pass_key, MTDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<MTSampler> MTSampler::CreateSampler(MTDevice& device, const SamplerDesc& desc)
{
    MTLSamplerDescriptor* sampler_descriptor = [MTLSamplerDescriptor new];
    sampler_descriptor.minFilter = ConvertToSamplerMinMagFilter(desc.min_filter);
    sampler_descriptor.magFilter = ConvertToSamplerMinMagFilter(desc.mag_filter);
    sampler_descriptor.mipFilter = ConvertToSamplerMipFilter(desc.mip_filter);
    if (desc.anisotropy_enable) {
        sampler_descriptor.maxAnisotropy = desc.max_anisotropy;
    }
    sampler_descriptor.sAddressMode = ConvertToSamplerAddressMode(desc.address_mode_u);
    sampler_descriptor.tAddressMode = ConvertToSamplerAddressMode(desc.address_mode_v);
    sampler_descriptor.rAddressMode = ConvertToSamplerAddressMode(desc.address_mode_w);
    sampler_descriptor.borderColor = ConvertToSamplerBorderColor(desc.border_color);
    sampler_descriptor.reductionMode = ConvertToSamplerReductionMode(desc.reduction_mode);
    sampler_descriptor.lodMinClamp = desc.min_lod;
    sampler_descriptor.lodMaxClamp = desc.max_lod;
    sampler_descriptor.lodBias = desc.mip_lod_bias;
    if (desc.compare_enable) {
        sampler_descriptor.compareFunction = ConvertToCompareFunction(desc.compare_func);
    }
    sampler_descriptor.supportArgumentBuffers = YES;

    std::shared_ptr<MTSampler> self = std::make_shared<MTSampler>(PassKey<MTSampler>(), device);
    self->resource_type_ = ResourceType::kSampler;
    self->sampler_ = [device.GetDevice() newSamplerStateWithDescriptor:sampler_descriptor];
    return self;
}

void MTSampler::SetName(const std::string& name)
{
    // It is not possible to set the name after MTLSamplerState creation, because label is readonly property.
}

id<MTLSamplerState> MTSampler::GetSampler() const
{
    return sampler_;
}
