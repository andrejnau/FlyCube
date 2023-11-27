#pragma once
#include "Instance/BaseTypes.h"
#include "Memory/Memory.h"

#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

class DXMemory : public Memory {
public:
    DXMemory(DXDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits);
    MemoryType GetMemoryType() const override;
    ComPtr<ID3D12Heap> GetHeap() const;

private:
    MemoryType m_memory_type;
    ComPtr<ID3D12Heap> m_heap;
};
