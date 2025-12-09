#include "Resource/DXBuffer.h"

#include "Device/DXDevice.h"
#include "Memory/DXMemory.h"
#include "Utilities/Common.h"

#include <directx/d3dx12.h>
#include <nowide/convert.hpp>

DXBuffer::DXBuffer(PassKey<DXBuffer> pass_key, DXDevice& device)
    : device_(device)
{
}

// static
std::shared_ptr<DXBuffer> DXBuffer::CreateBuffer(DXDevice& device, const BufferDesc& desc)
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
    if (desc.usage & BindFlag::kUnorderedAccess) {
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (desc.usage & BindFlag::kAccelerationStructure) {
        state = ResourceState::kRaytracingAccelerationStructure;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    std::shared_ptr<DXBuffer> self = std::make_shared<DXBuffer>(PassKey<DXBuffer>(), device);
    self->resource_type_ = ResourceType::kBuffer;
    self->resource_desc_ = resource_desc;
    self->SetInitialState(state);
    return self;
}

void DXBuffer::CommitMemory(MemoryType memory_type)
{
    memory_type_ = memory_type;

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
                                                 ConvertState(GetInitialState()), nullptr, IID_PPV_ARGS(&resource_));
}

void DXBuffer::BindMemory(const std::shared_ptr<Memory>& memory, uint64_t offset)
{
    memory_type_ = memory->GetMemoryType();

    // TODO
    if (memory_type_ == MemoryType::kUpload) {
        SetInitialState(ResourceState::kGenericRead);
    }

    decltype(auto) dx_memory = memory->As<DXMemory>();
    device_.GetDevice()->CreatePlacedResource(dx_memory.GetHeap().Get(), offset, &resource_desc_,
                                              ConvertState(GetInitialState()), nullptr, IID_PPV_ARGS(&resource_));
}

MemoryRequirements DXBuffer::GetMemoryRequirements() const
{
    D3D12_RESOURCE_ALLOCATION_INFO allocation_info =
        device_.GetDevice()->GetResourceAllocationInfo(0, 1, &resource_desc_);
    return { allocation_info.SizeInBytes, allocation_info.Alignment, 0 };
}

uint64_t DXBuffer::GetWidth() const
{
    return resource_desc_.Width;
}

void DXBuffer::SetName(const std::string& name)
{
    if (resource_) {
        resource_->SetName(nowide::widen(name).c_str());
    }
}

uint8_t* DXBuffer::Map()
{
    CD3DX12_RANGE range(0, 0);
    uint8_t* dst_data = nullptr;
    CHECK_HRESULT(resource_->Map(0, &range, reinterpret_cast<void**>(&dst_data)));
    return dst_data;
}

void DXBuffer::Unmap()
{
    CD3DX12_RANGE range(0, 0);
    resource_->Unmap(0, &range);
}

ID3D12Resource* DXBuffer::GetResource() const
{
    return resource_.Get();
}

const D3D12_RESOURCE_DESC& DXBuffer::GetResourceDesc() const
{
    return resource_desc_;
}
