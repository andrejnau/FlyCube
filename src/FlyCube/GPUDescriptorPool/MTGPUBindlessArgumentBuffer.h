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
    void AddAllocation(uint32_t offset, id<MTLAllocation> allocation);
    id<MTLResidencySet> GetResidencySet() const;

private:
    void ResizeHeap(uint32_t req_size);

    MTDevice& device_;
    uint32_t size_ = 0;
    uint32_t offset_ = 0;
    std::multimap<uint32_t, uint32_t> empty_ranges_;
    id<MTLBuffer> buffer_ = nullptr;
    id<MTLResidencySet> residency_set_ = nullptr;
    std::vector<id<MTLAllocation>> allocations_;
    std::map<id<MTLAllocation>, size_t> allocations_cnt_;
};
