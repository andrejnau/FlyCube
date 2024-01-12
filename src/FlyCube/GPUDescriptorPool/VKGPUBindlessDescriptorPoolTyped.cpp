#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"

#include "Device/VKDevice.h"

#include <stdexcept>

VKGPUBindlessDescriptorPoolTyped::VKGPUBindlessDescriptorPoolTyped(VKDevice& device, vk::DescriptorType type)
    : m_device(device)
    , m_type(type)
{
}

void VKGPUBindlessDescriptorPoolTyped::ResizeHeap(uint32_t req_size)
{
    req_size = std::min(req_size, m_device.GetMaxDescriptorSetBindings(m_type));

    if (m_size >= req_size) {
        return;
    }

    Descriptor descriptor;

    vk::DescriptorPoolSize pool_size = {};
    pool_size.type = m_type;
    pool_size.descriptorCount = req_size;

    vk::DescriptorPoolCreateInfo pool_info = {};
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    descriptor.pool = m_device.GetDevice().createDescriptorPoolUnique(pool_info);

    vk::DescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = m_type;
    binding.descriptorCount = m_device.GetMaxDescriptorSetBindings(binding.descriptorType);
    binding.stageFlags = vk::ShaderStageFlagBits::eAll;

    vk::DescriptorBindingFlags binding_flag = vk::DescriptorBindingFlagBits::eVariableDescriptorCount;

    vk::DescriptorSetLayoutBindingFlagsCreateInfo layout_flags_info = {};
    layout_flags_info.bindingCount = 1;
    layout_flags_info.pBindingFlags = &binding_flag;

    vk::DescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;
    layout_info.pNext = &layout_flags_info;

    descriptor.set_layout = m_device.GetDevice().createDescriptorSetLayoutUnique(layout_info);

    vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_info = {};
    variable_descriptor_count_info.descriptorSetCount = 1;
    variable_descriptor_count_info.pDescriptorCounts = &req_size;

    vk::DescriptorSetAllocateInfo alloc_info = {};
    alloc_info.descriptorPool = descriptor.pool.get();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor.set_layout.get();
    alloc_info.pNext = &variable_descriptor_count_info;

    descriptor.set = std::move(m_device.GetDevice().allocateDescriptorSetsUnique(alloc_info).front());

    if (m_size) {
        vk::CopyDescriptorSet copy_descriptors;
        copy_descriptors.srcSet = m_descriptor.set.get();
        copy_descriptors.dstSet = descriptor.set.get();
        copy_descriptors.descriptorCount = m_size;
        m_device.GetDevice().updateDescriptorSets(0, nullptr, 1, &copy_descriptors);
    }

    m_size = req_size;

    m_descriptor.set.release();
    m_descriptor = std::move(descriptor);
}

VKGPUDescriptorPoolRange VKGPUBindlessDescriptorPoolTyped::Allocate(uint32_t count)
{
    auto it = m_empty_ranges.lower_bound(count);
    if (it != m_empty_ranges.end()) {
        size_t offset = it->second;
        size_t size = it->first;
        m_empty_ranges.erase(it);
        return VKGPUDescriptorPoolRange(*this, offset, size);
    }
    if (m_offset + count > m_size) {
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
        if (m_offset + count > m_size) {
            throw std::runtime_error("Failed to resize VKGPUBindlessDescriptorPoolTyped");
        }
    }
    m_offset += count;
    return VKGPUDescriptorPoolRange(*this, m_offset - count, count);
}

void VKGPUBindlessDescriptorPoolTyped::OnRangeDestroy(uint32_t offset, uint32_t size)
{
    m_empty_ranges.emplace(size, offset);
}

vk::DescriptorSet VKGPUBindlessDescriptorPoolTyped::GetDescriptorSet() const
{
    return m_descriptor.set.get();
}
