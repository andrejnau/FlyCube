#pragma once
#include "GPUDescriptorPool/MTGPUArgumentBufferRange.h"

#import <Metal/Metal.h>

#include <algorithm>
#include <map>

class MTDevice;

class MTGPUBindlessArgumentBuffer {
public:
    MTGPUBindlessArgumentBuffer(MTDevice& device);
    MTGPUArgumentBufferRange Allocate(uint32_t count);
    void OnRangeDestroy(uint32_t offset, uint32_t size);
    id<MTLBuffer> GetArgumentBuffer() const;

private:
    void ResizeHeap(uint32_t req_size);

    MTDevice& m_device;
    uint32_t m_size = 0;
    uint32_t m_offset = 0;
    std::multimap<uint32_t, uint32_t> m_empty_ranges;
    id<MTLBuffer> m_buffer;
};
