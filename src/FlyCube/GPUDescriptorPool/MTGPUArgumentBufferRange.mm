#include "GPUDescriptorPool/MTGPUArgumentBufferRange.h"

#include "GPUDescriptorPool/MTGPUBindlessArgumentBuffer.h"

MTGPUArgumentBufferRange::MTGPUArgumentBufferRange(MTGPUBindlessArgumentBuffer& argument_buffer,
                                                   uint32_t offset,
                                                   uint32_t size)
    : argument_buffer_(argument_buffer)
    , offset_(offset)
    , size_(size)
    , callback_(this, [offset_ = offset_, size_ = size_, argument_buffer_ = argument_buffer_](auto) {
        argument_buffer_.get().OnRangeDestroy(offset_, size_);
    })
{
}

id<MTLBuffer> MTGPUArgumentBufferRange::GetArgumentBuffer() const
{
    return argument_buffer_.get().GetArgumentBuffer();
}

uint32_t MTGPUArgumentBufferRange::GetOffset() const
{
    return offset_;
}

void MTGPUArgumentBufferRange::AddAllocation(uint32_t offset, id<MTLAllocation> allocation)
{
    argument_buffer_.get().AddAllocation(offset, allocation);
}
