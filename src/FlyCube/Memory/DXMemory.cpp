#include "Memory/DXMemory.h"

#include "Device/DXDevice.h"

#include <directx/d3dx12.h>

DXMemory::DXMemory(DXDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
    : memory_type_(memory_type)
{
    D3D12_HEAP_DESC desc = {};
    desc.Properties = CD3DX12_HEAP_PROPERTIES(GetHeapType(memory_type));
    desc.SizeInBytes = size;
    desc.Alignment = memory_type_bits;
    if (device.IsCreateNotZeroedAvailable()) {
        desc.Flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
    }
    CHECK_HRESULT(device.GetDevice()->CreateHeap(&desc, IID_PPV_ARGS(&heap_)));
}

MemoryType DXMemory::GetMemoryType() const
{
    return memory_type_;
}

ComPtr<ID3D12Heap> DXMemory::GetHeap() const
{
    return heap_;
}
