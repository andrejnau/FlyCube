#include "Resource/DXResource.h"

#include "Device/DXDevice.h"
#include "Memory/DXMemory.h"
#include "Utilities/DXGIFormatHelper.h"
#include "Utilities/SystemUtils.h"

#include <directx/d3dx12.h>
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

DXResource::DXResource(DXDevice& device)
    : m_device(device)
{
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

    m_device.GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(GetHeapType(m_memory_type)), flags, &desc,
                                                  ConvertState(GetInitialState()), p_clear_value,
                                                  IID_PPV_ARGS(&resource));
}

void DXResource::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    m_memory = memory;
    m_memory_type = m_memory->GetMemoryType();
    auto clear_value = GetClearValue(desc);
    D3D12_CLEAR_VALUE* p_clear_value = nullptr;
    if (clear_value.has_value()) {
        p_clear_value = &clear_value.value();
    }

    // TODO
    if (m_memory_type == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    }

    decltype(auto) dx_memory = m_memory->As<DXMemory>();
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

bool DXResource::AllowCommonStatePromotion(ResourceState state_after)
{
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        return true;
    }
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) {
        switch (ConvertState(state_after)) {
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:
            return false;
        default:
            return true;
        }
    } else {
        switch (ConvertState(state_after)) {
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_COPY_DEST:
        case D3D12_RESOURCE_STATE_COPY_SOURCE:
            return true;
        default:
            return false;
        }
    }
    return false;
}

MemoryRequirements DXResource::GetMemoryRequirements() const
{
    D3D12_RESOURCE_ALLOCATION_INFO allocation_info = m_device.GetDevice()->GetResourceAllocationInfo(0, 1, &desc);
    return { allocation_info.SizeInBytes, allocation_info.Alignment, 0 };
}
