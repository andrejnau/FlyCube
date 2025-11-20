#include "QueryHeap/MTQueryHeap.h"

#include "Device/MTDevice.h"

MTQueryHeap::MTQueryHeap(MTDevice& device, QueryHeapType type, uint32_t count)
{
    buffer_ = [device.GetDevice() newBufferWithLength:count * sizeof(uint64_t) options:MTLResourceStorageModePrivate];
}

QueryHeapType MTQueryHeap::GetType() const
{
    return QueryHeapType::kAccelerationStructureCompactedSize;
}

id<MTLBuffer> MTQueryHeap::GetBuffer() const
{
    return buffer_;
}
