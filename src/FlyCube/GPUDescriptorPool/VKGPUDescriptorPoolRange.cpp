#include "GPUDescriptorPool/VKGPUDescriptorPoolRange.h"

#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"

VKGPUDescriptorPoolRange::VKGPUDescriptorPoolRange(VKGPUBindlessDescriptorPoolTyped& pool,
                                                   vk::DescriptorSet descriptor_set,
                                                   uint32_t offset,
                                                   uint32_t size,
                                                   vk::DescriptorType type)
    : m_pool(pool)
    , m_descriptor_set(descriptor_set)
    , m_offset(offset)
    , m_size(size)
    , m_type(type)
    , m_callback(this, [m_offset = m_offset, m_size = m_size, m_pool = m_pool](auto) {
        m_pool.get().OnRangeDestroy(m_offset, m_size);
    })
{
}

vk::DescriptorSet VKGPUDescriptorPoolRange::GetDescriptoSet() const
{
    return m_descriptor_set;
}

uint32_t VKGPUDescriptorPoolRange::GetOffset() const
{
    return m_offset;
}
