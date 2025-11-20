#pragma once
#include "Instance/BaseTypes.h"
#include "Memory/Memory.h"

#import <Metal/Metal.h>

MTLStorageMode ConvertStorageMode(MemoryType memory_type);

class MTDevice;

class MTMemory : public Memory {
public:
    MTMemory(MTDevice& device, uint64_t size, MemoryType memory_type);
    MemoryType GetMemoryType() const override;

    id<MTLHeap> GetHeap() const;

private:
    MemoryType memory_type_;
    id<MTLHeap> heap_ = nullptr;
};
