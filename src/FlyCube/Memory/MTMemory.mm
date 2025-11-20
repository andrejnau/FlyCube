#include "Memory/MTMemory.h"

#include "Device/MTDevice.h"
#include "Utilities/NotReached.h"

MTLStorageMode ConvertStorageMode(MemoryType memory_type)
{
    switch (memory_type) {
    case MemoryType::kDefault:
        return MTLStorageModePrivate;
    case MemoryType::kUpload:
    case MemoryType::kReadback:
        return MTLStorageModeShared;
    default:
        NOTREACHED();
    }
}

MTMemory::MTMemory(MTDevice& device, uint64_t size, MemoryType memory_type)
    : memory_type_(memory_type)
{
    MTLHeapDescriptor* heap_descriptor = [MTLHeapDescriptor new];
    heap_descriptor.size = size;
    heap_descriptor.storageMode = ConvertStorageMode(memory_type);
    heap_descriptor.cpuCacheMode = MTLCPUCacheModeDefaultCache;
    heap_descriptor.hazardTrackingMode = MTLHazardTrackingModeTracked;
    heap_descriptor.type = MTLHeapTypePlacement;
    heap_ = [device.GetDevice() newHeapWithDescriptor:heap_descriptor];
}

MemoryType MTMemory::GetMemoryType() const
{
    return memory_type_;
}

id<MTLHeap> MTMemory::GetHeap() const
{
    return heap_;
}
