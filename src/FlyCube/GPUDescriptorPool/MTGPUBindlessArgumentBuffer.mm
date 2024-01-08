#include "GPUDescriptorPool/MTGPUBindlessArgumentBuffer.h"

#include "Device/MTDevice.h"

MTGPUBindlessArgumentBuffer::MTGPUBindlessArgumentBuffer(MTDevice& device)
    : m_device(device)
{
}

void MTGPUBindlessArgumentBuffer::ResizeHeap(uint32_t req_size)
{
    if (m_size >= req_size) {
        return;
    }

    id<MTLBuffer> buffer = [m_device.GetDevice() newBufferWithLength:req_size * sizeof(uint64_t)
                                                             options:MTLResourceStorageModeShared];

    if (m_size && m_buffer) {
        auto queue = m_device.GetMTCommandQueue();
        auto command_buffer = [queue commandBuffer];
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        [blit_encoder copyFromBuffer:m_buffer sourceOffset:0 toBuffer:buffer destinationOffset:0 size:m_size];
        [blit_encoder endEncoding];
        [command_buffer commit];
    }

    m_size = req_size;
    m_buffer = buffer;
    m_use_resources.resize(m_size);
}

MTGPUArgumentBufferRange MTGPUBindlessArgumentBuffer::Allocate(uint32_t count)
{
    auto it = m_empty_ranges.lower_bound(count);
    if (it != m_empty_ranges.end()) {
        size_t offset = it->second;
        size_t size = it->first;
        m_empty_ranges.erase(it);
        return MTGPUArgumentBufferRange(*this, offset, size);
    }
    if (m_offset + count > m_size) {
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    }
    m_offset += count;
    return MTGPUArgumentBufferRange(*this, m_offset - count, count);
}

void MTGPUBindlessArgumentBuffer::OnRangeDestroy(uint32_t offset, uint32_t size)
{
    m_empty_ranges.emplace(size, offset);
    for (uint32_t i = offset; i < offset + size; ++i) {
        m_use_resources[i] = {};
    }
}

id<MTLBuffer> MTGPUBindlessArgumentBuffer::GetArgumentBuffer() const
{
    return m_buffer;
}

void MTGPUBindlessArgumentBuffer::SetResourceUsage(uint32_t offset, id<MTLResource> resource, MTLResourceUsage usage)
{
    m_use_resources[offset] = { resource, usage };
}

const std::vector<std::pair<id<MTLResource>, MTLResourceUsage>>& MTGPUBindlessArgumentBuffer::GetResourcesUsage() const
{
    return m_use_resources;
}
