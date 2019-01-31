#include "Context/VKDescriptorPool.h"
#include "Context/VKContext.h"

VKDescriptorPool::VKDescriptorPool(VKContext & context)
    : m_context(context)
{
}

void VKDescriptorPool::ResizeHeap(const std::map<VkDescriptorType, size_t>& count)
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto & x : count)
    {
        pool_sizes.emplace_back();
        VkDescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = x.first;
        pool_size.descriptorCount = x.second;
    }

    // TODO: fix me
    if (count.empty())
    {
        pool_sizes.emplace_back();
        VkDescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        pool_size.descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = pool_sizes.size();
    poolInfo.pPoolSizes = pool_sizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(m_context.m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VkDescriptorSet VKDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout & set_layout, const std::map<VkDescriptorType, size_t>& count)
{
    ResizeHeap(count);

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

void VKDescriptorPool::OnFrameBegin()
{
}
