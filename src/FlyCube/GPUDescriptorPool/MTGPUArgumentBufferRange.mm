#include "GPUDescriptorPool/MTGPUArgumentBufferRange.h"

#include "GPUDescriptorPool/MTGPUBindlessArgumentBuffer.h"

MTGPUArgumentBufferRange::MTGPUArgumentBufferRange(MTGPUBindlessArgumentBuffer& argument_buffer,
                                                   uint32_t offset,
                                                   uint32_t size)
    : m_argument_buffer(argument_buffer)
    , m_offset(offset)
    , m_size(size)
    , m_callback(this, [m_offset = m_offset, m_size = m_size, m_argument_buffer = m_argument_buffer](auto) {
        m_argument_buffer.get().OnRangeDestroy(m_offset, m_size);
    })
{
}

id<MTLBuffer> MTGPUArgumentBufferRange::GetArgumentBuffer() const
{
    return m_argument_buffer.get().GetArgumentBuffer();
}

uint32_t MTGPUArgumentBufferRange::GetOffset() const
{
    return m_offset;
}

void MTGPUArgumentBufferRange::SetResourceUsage(uint32_t offset, id<MTLResource> resource, MTLResourceUsage usage)
{
    m_argument_buffer.get().SetResourceUsage(offset, resource, usage);
}
