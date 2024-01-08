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
    void SetResourceUsage(uint32_t offset, id<MTLResource> resource, MTLResourceUsage usage);
    const std::vector<std::pair<id<MTLResource>, MTLResourceUsage>>& GetResourcesUsage() const;

private:
    void ResizeHeap(uint32_t req_size);

    MTDevice& m_device;
    uint32_t m_size = 0;
    uint32_t m_offset = 0;
    std::multimap<uint32_t, uint32_t> m_empty_ranges;
    id<MTLBuffer> m_buffer;
    std::vector<std::pair<id<MTLResource>, MTLResourceUsage>> m_use_resources;
};
