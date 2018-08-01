#include "Context/VKDescriptorPool.h"
#include "Context/VKContext.h"

VKDescriptorPool::VKDescriptorPool(VKContext & context)
    : m_context(context)
{
}

void VKDescriptorPool::ResizeHeap()
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto & x : m_size_by_type)
    {
        pool_sizes.emplace_back();
        VkDescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = x.first;
        pool_size.descriptorCount = x.second;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = pool_sizes.size();
    poolInfo.pPoolSizes = pool_sizes.data();
    poolInfo.maxSets = m_max_descriptor_sets;

    if (vkCreateDescriptorPool(m_context.m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VkDescriptorSet VKDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout & set_layout)
{
    if (first_frame_alloc)
    {
        ResizeHeap();
        first_frame_alloc = false;
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &set_layout;

    VkDescriptorSet m_descriptor_set;
    if (vkAllocateDescriptorSets(m_context.m_device, &allocInfo, &m_descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    return m_descriptor_set;
}

void VKDescriptorPool::ReqFrameDescriptionDrawCalls(size_t count)
{
    m_max_descriptor_sets += count;
}

void VKDescriptorPool::ReqFrameDescription(VkDescriptorType type, size_t count)
{
    m_size_by_type[type] += count;
}

void VKDescriptorPool::OnFrameBegin()
{
    first_frame_alloc = true;
    m_max_descriptor_sets = 0;
    m_size_by_type.clear();
}
