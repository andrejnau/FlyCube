#include "Memory/MTMemory.h"

#include "Device/MTDevice.h"

MTLStorageMode ConvertStorageMode(MemoryType memory_type)
{
    switch (memory_type) {
    case MemoryType::kDefault:
        return MTLStorageModePrivate;
    case MemoryType::kUpload:
    case MemoryType::kReadback:
        return MTLStorageModeShared;
    default:
        assert(false);
        return MTLStorageModeShared;
    }
}

MTMemory::MTMemory(MTDevice& device, uint64_t size, MemoryType memory_type, uint32_t memory_type_bits)
    : m_memory_type(memory_type)
{
    MTLHeapDescriptor* heap_descriptor = [[MTLHeapDescriptor alloc] init];
    heap_descriptor.size = size;
    heap_descriptor.storageMode = ConvertStorageMode(memory_type);
    heap_descriptor.cpuCacheMode = MTLCPUCacheModeDefaultCache;
    heap_descriptor.hazardTrackingMode = MTLHazardTrackingModeTracked;
    heap_descriptor.type = MTLHeapTypePlacement;
    m_heap = [device.GetDevice() newHeapWithDescriptor:heap_descriptor];
}

MemoryType MTMemory::GetMemoryType() const
{
    return m_memory_type;
}

id<MTLHeap> MTMemory::GetHeap() const
{
    return m_heap;
}
