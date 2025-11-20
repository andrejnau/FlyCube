#pragma once
#import <Metal/Metal.h>

#include <functional>
#include <memory>

class MTGPUBindlessArgumentBuffer;

class MTGPUArgumentBufferRange {
public:
    MTGPUArgumentBufferRange(MTGPUBindlessArgumentBuffer& argument_buffer, uint32_t offset, uint32_t size);
    id<MTLBuffer> GetArgumentBuffer() const;
    uint32_t GetOffset() const;
    void AddAllocation(uint32_t offset, id<MTLAllocation> allocation);

private:
    std::reference_wrapper<MTGPUBindlessArgumentBuffer> argument_buffer_;
    uint32_t offset_;
    uint32_t size_;
    std::unique_ptr<MTGPUArgumentBufferRange, std::function<void(MTGPUArgumentBufferRange*)>> callback_;
};
