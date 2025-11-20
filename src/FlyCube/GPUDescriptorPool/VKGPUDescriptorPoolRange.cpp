#include "GPUDescriptorPool/VKGPUDescriptorPoolRange.h"

#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"

VKGPUDescriptorPoolRange::VKGPUDescriptorPoolRange(VKGPUBindlessDescriptorPoolTyped& pool,
                                                   uint32_t offset,
                                                   uint32_t size)
    : pool_(pool)
    , offset_(offset)
    , size_(size)
    , callback_(this,
                [offset_ = offset_, size_ = size_, pool_ = pool_](auto) { pool_.get().OnRangeDestroy(offset_, size_); })
{
}

vk::DescriptorSet VKGPUDescriptorPoolRange::GetDescriptorSet() const
{
    return pool_.get().GetDescriptorSet();
}

uint32_t VKGPUDescriptorPoolRange::GetOffset() const
{
    return offset_;
}
