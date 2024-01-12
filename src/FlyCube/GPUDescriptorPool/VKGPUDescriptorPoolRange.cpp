#include "GPUDescriptorPool/VKGPUDescriptorPoolRange.h"

#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"

VKGPUDescriptorPoolRange::VKGPUDescriptorPoolRange(VKGPUBindlessDescriptorPoolTyped& pool,
                                                   uint32_t offset,
                                                   uint32_t size)
    : m_pool(pool)
    , m_offset(offset)
    , m_size(size)
    , m_callback(this, [m_offset = m_offset, m_size = m_size, m_pool = m_pool](auto) {
        m_pool.get().OnRangeDestroy(m_offset, m_size);
    })
{
}

vk::DescriptorSet VKGPUDescriptorPoolRange::GetDescriptorSet() const
{
    return m_pool.get().GetDescriptorSet();
}

uint32_t VKGPUDescriptorPoolRange::GetOffset() const
{
    return m_offset;
}
