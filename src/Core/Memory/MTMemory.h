#pragma once
#include "Instance/BaseTypes.h"
#include "Memory/Memory.h"

#import <Metal/Metal.h>

MTLStorageMode ConvertStorageMode(MemoryType memory_type);

class MTDevice;

class MTMemory : public Memory {
public:
    MTMemory(MTDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits);
    MemoryType GetMemoryType() const override;

    id<MTLHeap> GetHeap() const;

private:
    MemoryType m_memory_type;
    id<MTLHeap> m_heap;
};
