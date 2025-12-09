#include "Resource/DXTexture.h"

#include "Device/DXDevice.h"
#include "Memory/DXMemory.h"
#include "Utilities/DXGIFormatHelper.h"
#include "Utilities/NotReached.h"

#include <directx/d3dx12.h>
#include <gli/dx.hpp>
#include <nowide/convert.hpp>

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

} // namespace

DXTexture::DXTexture(PassKey<DXTexture> pass_key, DXDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<DXTexture> DXTexture::WrapSwapchainBackBuffer(DXDevice& device,
                                                              ComPtr<ID3D12Resource> back_buffer,
                                                              gli::format format)
{
    std::shared_ptr<DXTexture> self = std::make_shared<DXTexture>(PassKey<DXTexture>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = format;
    self->resource_ = back_buffer;
    self->resource_desc_ = back_buffer->GetDesc();
    self->is_back_buffer_ = true;
    self->SetInitialState(ResourceState::kPresent);
    return self;
}

// static
std::shared_ptr<DXTexture> DXTexture::CreateTexture(DXDevice& device, const TextureDesc& desc)
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
    default:
        NOTREACHED();
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

    std::shared_ptr<DXTexture> self = std::make_shared<DXTexture>(PassKey<DXTexture>(), device);
    self->resource_type_ = ResourceType::kTexture;
    self->format_ = desc.format;
    self->resource_desc_ = resource_desc;
    self->SetInitialState(ResourceState::kCommon);
    return self;
}

void DXTexture::CommitMemory(MemoryType memory_type)
{
    memory_type_ = memory_type;
    auto clear_value = GetClearValue(resource_desc_);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
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

void DXTexture::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();
    auto clear_value = GetClearValue(resource_desc_);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    decltype(auto) dx_memory = memory->As<DXMemory>();
    device_.GetDevice()->CreatePlacedResource(dx_memory.GetHeap().Get(), offset, &resource_desc_,
                                              ConvertState(GetInitialState()), p_clear_value, IID_PPV_ARGS(&resource_));
}

MemoryRequirements DXTexture::GetMemoryRequirements() const
{
    D3D12_RESOURCE_ALLOCATION_INFO allocation_info =
        device_.GetDevice()->GetResourceAllocationInfo(0, 1, &resource_desc_);
    return { allocation_info.SizeInBytes, allocation_info.Alignment, 0 };
}

uint64_t DXTexture::GetWidth() const
{
    return resource_desc_.Width;
}

uint32_t DXTexture::GetHeight() const
{
    return resource_desc_.Height;
}

uint16_t DXTexture::GetLayerCount() const
{
    return resource_desc_.DepthOrArraySize;
}

uint16_t DXTexture::GetLevelCount() const
{
    return resource_desc_.MipLevels;
}

uint32_t DXTexture::GetSampleCount() const
{
    return resource_desc_.SampleDesc.Count;
}

void DXTexture::SetName(const std::string& name)
{
    if (resource_) {
        resource_->SetName(nowide::widen(name).c_str());
    }
}

ID3D12Resource* DXTexture::GetResource() const
{
    return resource_.Get();
}

const D3D12_RESOURCE_DESC& DXTexture::GetResourceDesc() const
{
    return resource_desc_;
}
