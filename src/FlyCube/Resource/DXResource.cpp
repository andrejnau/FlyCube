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
    self->m_resource = back_buffer;
    self->m_resource_desc = back_buffer->GetDesc();
    self->m_is_back_buffer = true;
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
    self->m_resource_type = ResourceType::kTexture;
    self->m_format = desc.format;
    self->m_resource_desc = resource_desc;
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
        buffer_size = (buffer_size + 255) & ~255;
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
    self->m_resource_type = ResourceType::kBuffer;
    self->m_resource_desc = resource_desc;
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
    self->m_sampler_desc = sampler_desc;
    return self;
}

// static
std::shared_ptr<DXResource> DXResource::CreateAccelerationStructure(DXDevice& device,
                                                                    const AccelerationStructureDesc& desc)
{
    std::shared_ptr<DXResource> self = std::make_shared<DXResource>(PassKey<DXResource>(), device);
    self->m_resource_type = ResourceType::kAccelerationStructure;
    self->m_acceleration_structure_address =
        desc.buffer->As<DXResource>().m_resource->GetGPUVirtualAddress() + desc.buffer_offset;
    return self;
}

void DXResource::CommitMemory(MemoryType memory_type)
{
    m_memory_type = memory_type;
    auto clear_value = GetClearValue(m_resource_desc);
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
    m_device.GetDevice()->CreateCommittedResource(&heap_properties, flags, &m_resource_desc,
                                                  ConvertState(GetInitialState()), p_clear_value,
                                                  IID_PPV_ARGS(&m_resource));
}

void DXResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory_type = memory->GetMemoryType();
    auto clear_value = GetClearValue(m_resource_desc);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    // TODO
    if (m_memory_type == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    }

    decltype(auto) dx_memory = memory->As<DXMemory>();
    m_device.GetDevice()->CreatePlacedResource(dx_memory.GetHeap().Get(), offset, &m_resource_desc,
                                               ConvertState(GetInitialState()), p_clear_value,
                                               IID_PPV_ARGS(&m_resource));
}

uint64_t DXResource::GetWidth() const
{
    return m_resource_desc.Width;
}

uint32_t DXResource::GetHeight() const
{
    return m_resource_desc.Height;
}

uint16_t DXResource::GetLayerCount() const
{
    return m_resource_desc.DepthOrArraySize;
}

uint16_t DXResource::GetLevelCount() const
{
    return m_resource_desc.MipLevels;
}

uint32_t DXResource::GetSampleCount() const
{
    return m_resource_desc.SampleDesc.Count;
}

uint64_t DXResource::GetAccelerationStructureHandle() const
{
    return m_acceleration_structure_address;
}

void DXResource::SetName(const std::string& name)
{
    if (m_resource) {
        m_resource->SetName(nowide::widen(name).c_str());
    }
}

uint8_t* DXResource::Map()
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    CHECK_HRESULT(m_resource->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    return dst_data;
}

void DXResource::Unmap()
{
    CD3DX12_RANGE range(0, 0);
    m_resource->Unmap(0, &range);
}

MemoryRequirements DXResource::GetMemoryRequirements() const
{
    D3D12_RESOURCE_ALLOCATION_INFO allocation_info =
        m_device.GetDevice()->GetResourceAllocationInfo(0, 1, &m_resource_desc);
    return { allocation_info.SizeInBytes, allocation_info.Alignment, 0 };
}

ID3D12Resource* DXResource::GetResource() const
{
    return m_resource.Get();
}

const D3D12_RESOURCE_DESC& DXResource::GetResourceDesc() const
{
    return m_resource_desc;
}

const D3D12_SAMPLER_DESC& DXResource::GetSamplerDesc() const
{
    return m_sampler_desc;
}

D3D12_GPU_VIRTUAL_ADDRESS DXResource::GetAccelerationStructureAddress() const
{
    return m_acceleration_structure_address;
}
