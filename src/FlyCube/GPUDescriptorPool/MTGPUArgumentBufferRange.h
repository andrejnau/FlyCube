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
    void SetResourceUsage(uint32_t offset, id<MTLResource> resource, MTLResourceUsage usage);

private:
    std::reference_wrapper<MTGPUBindlessArgumentBuffer> m_argument_buffer;
    uint32_t m_offset;
    uint32_t m_size;
    std::unique_ptr<MTGPUArgumentBufferRange, std::function<void(MTGPUArgumentBufferRange*)>> m_callback;
};
