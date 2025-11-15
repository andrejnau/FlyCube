#include "GPUDescriptorPool/MTGPUBindlessArgumentBuffer.h"

#include "Device/MTDevice.h"

MTGPUBindlessArgumentBuffer::MTGPUBindlessArgumentBuffer(MTDevice& device)
    : m_device(device)
    , m_residency_set(device.CreateResidencySet())
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
        memcpy(buffer.contents, m_buffer.contents, m_size * sizeof(uint64_t));
    }

    m_size = req_size;
    m_buffer = buffer;
    m_allocations.resize(m_size);
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
        if (!m_allocations[i]) {
            continue;
        }

        if (--m_allocations_cnt.at(m_allocations[i]) == 0) {
            [m_residency_set removeAllocation:m_allocations[i]];
            m_allocations_cnt.erase(m_allocations[i]);
        }
        m_allocations[i] = {};
    }
}

id<MTLBuffer> MTGPUBindlessArgumentBuffer::GetArgumentBuffer() const
{
    return m_buffer;
}

void MTGPUBindlessArgumentBuffer::AddAllocation(uint32_t offset, id<MTLAllocation> allocation)
{
    if (!allocation) {
        return;
    }

    if (++m_allocations_cnt[allocation] == 1) {
        [m_residency_set addAllocation:allocation];
    }
    m_allocations[offset] = allocation;
}

id<MTLResidencySet> MTGPUBindlessArgumentBuffer::GetResidencySet() const
{
    return m_residency_set;
}
