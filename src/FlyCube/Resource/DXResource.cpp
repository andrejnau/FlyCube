#include "Resource/DXResource.h"

#include "Device/DXDevice.h"
#include "Memory/DXMemory.h"
#include "Utilities/DXGIFormatHelper.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3dx12.h>
#include <gli/dx.hpp>
#include <nowide/convert.hpp>

#include <optional>

std::optional<D3D12_CLEAR_VALUE> GetClearValue(const D3D12_RESOURCE_DESC& desc)
{
    D3D12_CLEAR_VALUE clear_value = {};
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
        clear_value.Format = desc.Format;
        if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
            clear_value.Color[0] = 0.0f;
            clear_value.Color[1] = 0.0f;
            clear_value.Color[2] = 0.0f;
            clear_value.Color[3] = 1.0f;
            return clear_value;
        } else if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
            clear_value.DepthStencil.Depth = 1.0f;
            clear_value.DepthStencil.Stencil = 0;
            clear_value.Format = DepthStencilFromTypeless(clear_value.Format);
            return clear_value;
        }
    }
    return {};
}

DXResource::DXResource(PassKey<DXResource> pass_key, DXDevice& device)
    : m_device(device)
{
}

// static
std::shared_ptr<DXResource> DXResource::WrapSwapchainBackBuffer(DXDevice& device,
                                                                ComPtr<ID3D12Resource> back_buffer,
                                                                gli::format format)
{
    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->m_format = format;
    self->resource = back_buffer;
    self->desc = back_buffer->GetDesc();
    self->m_is_back_buffer = true;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateTexture(DXDevice& device,
                                                      TextureType type,
                                                      uint32_t bind_flag,
                                                      gli::format format,
                                                      uint32_t sample_count,
                                                      int width,
                                                      int height,
                                                      int depth,
                                                      int mip_levels)
{
    DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(format).DXGIFormat.DDS);
    if (bind_flag & BindFlag::kShaderResource) {
        dx_format = MakeTypelessDepthStencil(dx_format);
    }

    D3D12_RESOURCE_DESC desc = {};
    switch (type) {
    case TextureType::k1D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case TextureType::k2D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case TextureType::k3D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    }
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = depth;
    desc.MipLevels = mip_levels;
    desc.Format = dx_format;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_check_desc = {};
    ms_check_desc.Format = desc.Format;
    ms_check_desc.SampleCount = sample_count;
    device.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_check_desc,
                                            sizeof(ms_check_desc));
    desc.SampleDesc.Count = sample_count;
    desc.SampleDesc.Quality = ms_check_desc.NumQualityLevels - 1;

    if (bind_flag & BindFlag::kRenderTarget) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (bind_flag & BindFlag::kDepthStencil) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (bind_flag & BindFlag::kUnorderedAccess) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->m_resource_type = ResourceType::kTexture;
    self->m_format = format;
    self->desc = desc;
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateBuffer(DXDevice& device, uint32_t bind_flag, uint32_t buffer_size)
{
    if (buffer_size == 0) {
        return nullptr;
    }

    if (bind_flag & BindFlag::kConstantBuffer) {
        buffer_size = (buffer_size + 255) & ~255;
    }

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

    ResourceState state = ResourceState::kCommon;
    if (bind_flag & BindFlag::kRenderTarget) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (bind_flag & BindFlag::kDepthStencil) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (bind_flag & BindFlag::kUnorderedAccess) {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (bind_flag & BindFlag::kAccelerationStructure) {
        state = ResourceState::kRaytracingAccelerationStructure;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->m_resource_type = ResourceType::kBuffer;
    self->desc = desc;
    self->SetInitialState(state);
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateSampler(DXDevice& device, const SamplerDesc& desc)
{
    D3D12_SAMPLER_DESC sampler_desc = {};
    switch (desc.filter) {
    case SamplerFilter::kAnisotropic:
        sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case SamplerFilter::kMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case SamplerFilter::kComparisonMinMagMipLinear:
        sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    }

    switch (desc.mode) {
    case SamplerTextureAddressMode::kWrap:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case SamplerTextureAddressMode::kClamp:
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    }

    switch (desc.func) {
    case SamplerComparisonFunc::kNever:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case SamplerComparisonFunc::kAlways:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    case SamplerComparisonFunc::kLess:
        sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    }

    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = std::numeric_limits<float>::max();
    sampler_desc.MaxAnisotropy = 1;

    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->m_resource_type = ResourceType::kSampler;
    self->sampler_desc = sampler_desc;
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateAccelerationStructure(
    DXDevice& device,
    AccelerationStructureType type,
    const std::shared_ptr<Resource>& acceleration_structures_memory,
    uint64_t offset,
    uint64_t size)
{
    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->m_resource_type = ResourceType::kAccelerationStructure;
    self->acceleration_structure_handle =
        acceleration_structures_memory->As<DXResource>().resource->GetGPUVirtualAddress() + offset;
    return self;
}

void DXResource::CommitMemory(MemoryType memory_type)
{
    m_memory_type = memory_type;
    auto clear_value = GetClearValue(desc);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    // TODO
    if (m_memory_type == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    } else if (m_memory_type == MemoryType::kReadback) {
        SetInitialState(ResourceState::kCopyDest);
    }

    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
    if (m_device.IsCreateNotZeroedAvailable()) {
        flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
    }
    auto heap_properties = CD3DX12_HEAP_PROPERTIES(GetHeapType(m_memory_type));
    m_device.GetDevice()->CreateCommittedResource(&heap_properties, flags, &desc, ConvertState(GetInitialState()),
                                                  p_clear_value, IID_PPV_ARGS(&resource));
}

void DXResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory_type = memory->GetMemoryType();
    auto clear_value = GetClearValue(desc);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    // TODO
    if (m_memory_type == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    }

    decltype(auto) dx_memory = memory->As<DXMemory>();
    m_device.GetDevice()->CreatePlacedResource(dx_memory.GetHeap().Get(), offset, &desc,
                                               ConvertState(GetInitialState()), p_clear_value, IID_PPV_ARGS(&resource));
}

uint64_t DXResource::GetWidth() const
{
    return desc.Width;
}

uint32_t DXResource::GetHeight() const
{
    return desc.Height;
}

uint16_t DXResource::GetLayerCount() const
{
    return desc.DepthOrArraySize;
}

uint16_t DXResource::GetLevelCount() const
{
    return desc.MipLevels;
}

uint32_t DXResource::GetSampleCount() const
{
    return desc.SampleDesc.Count;
}

uint64_t DXResource::GetAccelerationStructureHandle() const
{
    return acceleration_structure_handle;
}

void DXResource::SetName(const std::string& name)
{
    if (resource) {
        resource->SetName(nowide::widen(name).c_str());
    }
}

uint8_t* DXResource::Map()
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    ASSERT_SUCCEEDED(resource->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    return dst_data;
}

void DXResource::Unmap()
{
    CD3DX12_RANGE range(0, 0);
    resource->Unmap(0, &range);
}

MemoryRequirements DXResource::GetMemoryRequirements() const
{
    D3D12_RESOURCE_ALLOCATION_INFO allocation_info = m_device.GetDevice()->GetResourceAllocationInfo(0, 1, &desc);
    return { allocation_info.SizeInBytes, allocation_info.Alignment, 0 };
}
