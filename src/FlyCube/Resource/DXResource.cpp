#include "Resource/DXResource.h"

#include "Device/DXDevice.h"
#include "Memory/DXMemory.h"
#include "Utilities/Common.h"
#include "Utilities/DXGIFormatHelper.h"
#include "Utilities/NotReached.h"

#include <directx/d3dx12.h>
#include <gli/dx.hpp>
#include <nowide/convert.hpp>

#include <algorithm>
#include <optional>
#include <span>

namespace {

std::optional<D3D12_CLEAR_VALUE> GetClearValue(const D3D12_RESOURCE_DESC& desc)
{
    D3D12_CLEAR_VALUE clear_value = {};
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
        clear_value.Format = desc.Format;
        if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
            clear_value.Color[0] = 0.0;
            clear_value.Color[1] = 0.0;
            clear_value.Color[2] = 0.0;
            clear_value.Color[3] = 1.0;
            return clear_value;
        } else if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
            clear_value.DepthStencil.Depth = 1.0;
            clear_value.DepthStencil.Stencil = 0;
            clear_value.Format = DepthStencilFromTypeless(clear_value.Format);
            return clear_value;
        }
    }
    return {};
}

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

DXResource::DXResource(PassKey<DXResource> pass_key, DXDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<DXResource> DXResource::WrapSwapchainBackBuffer(DXDevice& device,
                                                                ComPtr<ID3D12Resource> back_buffer,
                                                                gli::format format)
{
    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = format;
    self->resource_ = back_buffer;
    self->resource_desc_ = back_buffer->GetDesc();
    self->is_back_buffer_ = true;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateTexture(DXDevice& device, const TextureDesc& desc)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(desc.format).DXGIFormat.DDS);
    if (desc.usage & BindFlag::kShaderResource) {
        dx_format = MakeTypelessDepthStencil(dx_format);
    }

    D3D12_RESOURCE_DESC resource_desc = {};
    switch (desc.type) {
    case TextureType::k1D:
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case TextureType::k2D:
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case TextureType::k3D:
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    }
    resource_desc.Width = desc.width;
    resource_desc.Height = desc.height;
    resource_desc.DepthOrArraySize = desc.depth_or_array_layers;
    resource_desc.MipLevels = desc.mip_levels;
    resource_desc.Format = dx_format;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_check_desc = {};
    ms_check_desc.Format = resource_desc.Format;
    ms_check_desc.SampleCount = desc.sample_count;
    device.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_check_desc,
                                            sizeof(ms_check_desc));
    resource_desc.SampleDesc.Count = desc.sample_count;
    resource_desc.SampleDesc.Quality = ms_check_desc.NumQualityLevels - 1;

    if (desc.usage & BindFlag::kRenderTarget) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (desc.usage & BindFlag::kDepthStencil) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (desc.usage & BindFlag::kUnorderedAccess) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->resource_desc_ = resource_desc;
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateBuffer(DXDevice& device, const BufferDesc& desc)
{
    uint64_t buffer_size = desc.size;
    if (buffer_size == 0) {
        return nullptr;
    }

    if (desc.usage & BindFlag::kConstantBuffer) {
        buffer_size = Align(buffer_size, device.GetConstantBufferOffsetAlignment());
    }

    D3D12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

    ResourceState state = ResourceState::kCommon;
    if (desc.usage & BindFlag::kRenderTarget) {
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (desc.usage & BindFlag::kDepthStencil) {
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (desc.usage & BindFlag::kUnorderedAccess) {
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (desc.usage & BindFlag::kAccelerationStructure) {
        state = ResourceState::kRaytracingAccelerationStructure;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->resource_type_ = ResourceType::kBuffer;
    self->resource_desc_ = resource_desc;
    self->SetInitialState(state);
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateSampler(DXDevice& device, const SamplerDesc& desc)
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

    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->resource_type_ = ResourceType::kSampler;
    self->sampler_desc_ = sampler_desc;
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateAccelerationStructure(DXDevice& device,
                                                                    const AccelerationStructureDesc& desc)
{
    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->resource_type_ = ResourceType::kAccelerationStructure;
    self->acceleration_structure_address_ =
        desc.buffer->As<DXResource>().resource_->GetGPUVirtualAddress() + desc.buffer_offset;
    return self;
}

void DXResource::CommitMemory(MemoryType memory_type)
{
    memory_type_ = memory_type;
    auto clear_value = GetClearValue(resource_desc_);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    // TODO
    if (memory_type_ == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    } else if (memory_type_ == MemoryType::kReadback) {
        SetInitialState(ResourceState::kCopyDest);
    }

    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
    if (device_.IsCreateNotZeroedAvailable()) {
        flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
    }
    auto heap_properties = CD3DX12_HEAP_PROPERTIES(GetHeapType(memory_type_));
    device_.GetDevice()->CreateCommittedResource(&heap_properties, flags, &resource_desc_,
                                                 ConvertState(GetInitialState()), p_clear_value,
                                                 IID_PPV_ARGS(&resource_));
}

void DXResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    auto clear_value = GetClearValue(resource_desc_);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    // TODO
    if (memory_type_ == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    }

    decltype(auto) dx_memory = memory->As<DXMemory>();
    device_.GetDevice()->CreatePlacedResource(dx_memory.GetHeap().Get(), offset, &resource_desc_,
                                              ConvertState(GetInitialState()), p_clear_value, IID_PPV_ARGS(&resource_));
}

uint64_t DXResource::GetWidth() const
{
    return resource_desc_.Width;
}

uint32_t DXResource::GetHeight() const
{
    return resource_desc_.Height;
}

uint16_t DXResource::GetLayerCount() const
{
    return resource_desc_.DepthOrArraySize;
}

uint16_t DXResource::GetLevelCount() const
{
    return resource_desc_.MipLevels;
}

uint32_t DXResource::GetSampleCount() const
{
    return resource_desc_.SampleDesc.Count;
}

uint64_t DXResource::GetAccelerationStructureHandle() const
{
    return acceleration_structure_address_;
}

void DXResource::SetName(const std::string& name)
{
    if (resource_) {
        resource_->SetName(nowide::widen(name).c_str());
    }
}

uint8_t* DXResource::Map()
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    CHECK_HRESULT(resource_->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    return dst_data;
}

void DXResource::Unmap()
{
    CD3DX12_RANGE range(0, 0);
    resource_->Unmap(0, &range);
}

MemoryRequirements DXResource::GetMemoryRequirements() const
{
    D3D12_RESOURCE_ALLOCATION_INFO allocation_info =
        device_.GetDevice()->GetResourceAllocationInfo(0, 1, &resource_desc_);
    return { allocation_info.SizeInBytes, allocation_info.Alignment, 0 };
}

ID3D12Resource* DXResource::GetResource() const
{
    return resource_.Get();
}

const D3D12_RESOURCE_DESC& DXResource::GetResourceDesc() const
{
    return resource_desc_;
}

const D3D12_SAMPLER_DESC& DXResource::GetSamplerDesc() const
{
    return sampler_desc_;
}

D3D12_GPU_VIRTUAL_ADDRESS DXResource::GetAccelerationStructureAddress() const
{
    return acceleration_structure_address_;
}
