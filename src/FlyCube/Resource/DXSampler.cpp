#include "Resource/DXSampler.h"

#include "Device/DXDevice.h"
#include "Memory/DXMemory.h"
#include "Utilities/NotReached.h"

#include <algorithm>
#include <span>

namespace {

D3D12_FILTER_TYPE ConvertToFilterType(SamplerFilter filter)
{
    switch (filter) {
    case SamplerFilter::kNearest:
        return D3D12_FILTER_TYPE_POINT;
    case SamplerFilter::kLinear:
        return D3D12_FILTER_TYPE_LINEAR;
    default:
        NOTREACHED();
    }
}

D3D12_FILTER_REDUCTION_TYPE ConvertToFilterReductionType(SamplerReductionMode reduction_mode, bool compare_enable)
{
    switch (reduction_mode) {
    case SamplerReductionMode::kWeightedAverage:
        return compare_enable ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;
    case SamplerReductionMode::kMinimum:
        return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
    case SamplerReductionMode::kMaximum:
        return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
    default:
        NOTREACHED();
    }
}

D3D12_FILTER ConvertToFilter(const SamplerDesc& desc, bool is_aniso_filter_with_point_mip_supported)
{
    D3D12_FILTER_TYPE min_filter_type = ConvertToFilterType(desc.min_filter);
    D3D12_FILTER_TYPE mag_filter_type = ConvertToFilterType(desc.mag_filter);
    D3D12_FILTER_TYPE mip_filter_type = ConvertToFilterType(desc.mip_filter);
    D3D12_FILTER_REDUCTION_TYPE reduction_type = ConvertToFilterReductionType(desc.reduction_mode, desc.compare_enable);
    if (desc.anisotropy_enable) {
        if (min_filter_type == D3D12_FILTER_TYPE_POINT) {
            Logging::Println("SamplerFilter::kNearest min_filter is not supported, fallback to SamplerFilter::kLinear");
            min_filter_type = D3D12_FILTER_TYPE_LINEAR;
        }
        if (mag_filter_type == D3D12_FILTER_TYPE_POINT) {
            Logging::Println("SamplerFilter::kNearest mag_filter is not supported, fallback to SamplerFilter::kLinear");
            mag_filter_type = D3D12_FILTER_TYPE_LINEAR;
        }
        if (mip_filter_type == D3D12_FILTER_TYPE_POINT && !is_aniso_filter_with_point_mip_supported) {
            Logging::Println("SamplerFilter::kNearest mip_filter is not supported, fallback to SamplerFilter::kLinear");
            mip_filter_type = D3D12_FILTER_TYPE_LINEAR;
        }
    }
    D3D12_FILTER filter = D3D12_ENCODE_BASIC_FILTER(min_filter_type, mag_filter_type, mip_filter_type, reduction_type);
    if (desc.anisotropy_enable) {
        filter = static_cast<D3D12_FILTER>(filter | D3D12_ANISOTROPIC_FILTERING_BIT);
    }
    return filter;
}

D3D12_TEXTURE_ADDRESS_MODE ConvertToTextureAddressMode(SamplerAddressMode mode)
{
    switch (mode) {
    case SamplerAddressMode::kRepeat:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case SamplerAddressMode::kMirrorRepeat:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case SamplerAddressMode::kClampToEdge:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case SamplerAddressMode::kMirrorClampToEdge:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    case SamplerAddressMode::kClampToBorder:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    default:
        NOTREACHED();
    }
}

void InitBorderColor(std::span<float, 4> dst, SamplerBorderColor border_color)
{
    switch (border_color) {
    case SamplerBorderColor::kTransparentBlack: {
        static constexpr std::array<float, 4> kTransparentBlackValue = { 0.0, 0.0, 0.0, 0.0 };
        std::ranges::copy(kTransparentBlackValue, dst.begin());
        break;
    }
    case SamplerBorderColor::kOpaqueBlack: {
        static constexpr std::array<float, 4> kOpaqueBlackValue = { 0.0, 0.0, 0.0, 1.0 };
        std::ranges::copy(kOpaqueBlackValue, dst.begin());
        break;
    }
    case SamplerBorderColor::kOpaqueWhite: {
        static constexpr std::array<float, 4> kOpaqueWhiteValue = { 1.0, 1.0, 1.0, 1.0 };
        std::ranges::copy(kOpaqueWhiteValue, dst.begin());
        break;
    }
    default:
        NOTREACHED();
    }
}

} // namespace

DXSampler::DXSampler(PassKey<DXSampler> pass_key, DXDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<DXSampler> DXSampler::CreateSampler(DXDevice& device, const SamplerDesc& desc)
{
    D3D12_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = ConvertToFilter(desc, device.IsAnisoFilterWithPointMipSupported());
    sampler_desc.AddressU = ConvertToTextureAddressMode(desc.address_mode_u);
    sampler_desc.AddressV = ConvertToTextureAddressMode(desc.address_mode_v);
    sampler_desc.AddressW = ConvertToTextureAddressMode(desc.address_mode_w);
    sampler_desc.MipLODBias = desc.mip_lod_bias;
    sampler_desc.MaxAnisotropy = desc.max_anisotropy;
    sampler_desc.ComparisonFunc = ConvertToComparisonFunc(desc.compare_func);
    InitBorderColor(sampler_desc.BorderColor, desc.border_color);
    sampler_desc.MinLOD = desc.min_lod;
    sampler_desc.MaxLOD = desc.max_lod;

    std::shared_ptr<DXSampler> self = std::make_shared<DXSampler>(PassKey<DXSampler>(), device);
    self->resource_type_ = ResourceType::kSampler;
    self->sampler_desc_ = sampler_desc;
    return self;
}

void DXSampler::SetName(const std::string& name)
{
    // Sampler does not have name in DirectX 12
}

const D3D12_SAMPLER_DESC& DXSampler::GetSamplerDesc() const
{
    return sampler_desc_;
}
