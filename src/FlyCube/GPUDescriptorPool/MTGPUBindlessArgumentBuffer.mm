#include "GPUDescriptorPool/MTGPUBindlessArgumentBuffer.h"

#include "Device/MTDevice.h"

MTGPUBindlessArgumentBuffer::MTGPUBindlessArgumentBuffer(MTDevice& device)
    : device_(device)
    , residency_set_(device.CreateResidencySet())
{
}

void MTGPUBindlessArgumentBuffer::ResizeHeap(uint32_t req_size)
{
    if (size_ >= req_size) {
        return;
    }

    id<MTLBuffer> buffer = [device_.GetDevice() newBufferWithLength:req_size * sizeof(uint64_t)
                                                            options:MTLResourceStorageModeShared];
    if (size_ && buffer_) {
        memcpy(buffer.contents, buffer_.contents, size_ * sizeof(uint64_t));
    }

    size_ = req_size;
    buffer_ = buffer;
    allocations_.resize(size_);
}

MTGPUArgumentBufferRange MTGPUBindlessArgumentBuffer::Allocate(uint32_t count)
{
    auto it = empty_ranges_.lower_bound(count);
    if (it != empty_ranges_.end()) {
        size_t offset = it->second;
        size_t size = it->first;
        empty_ranges_.erase(it);
        return MTGPUArgumentBufferRange(*this, offset, size);
    }
    if (offset_ + count > size_) {
        ResizeHeap(std::max(offset_ + count, 2 * (size_ + 1)));
    }
    offset_ += count;
    return MTGPUArgumentBufferRange(*this, offset_ - count, count);
}

void MTGPUBindlessArgumentBuffer::OnRangeDestroy(uint32_t offset, uint32_t size)
{
    empty_ranges_.emplace(size, offset);
    for (uint32_t i = offset; i < offset + size; ++i) {
        if (!allocations_[i]) {
            continue;
        }

        if (--allocations_cnt_.at(allocations_[i]) == 0) {
            [residency_set_ removeAllocation:allocations_[i]];
            allocations_cnt_.erase(allocations_[i]);
        }
        allocations_[i] = {};
    }
}

id<MTLBuffer> MTGPUBindlessArgumentBuffer::GetArgumentBuffer() const
{
    return buffer_;
}

void MTGPUBindlessArgumentBuffer::AddAllocation(uint32_t offset, id<MTLAllocation> allocation)
{
    if (!allocation) {
        return;
    }

    if (++allocations_cnt_[allocation] == 1) {
        [residency_set_ addAllocation:allocation];
    }
    allocations_[offset] = allocation;
}

id<MTLResidencySet> MTGPUBindlessArgumentBuffer::GetResidencySet() const
{
    return residency_set_;
}
