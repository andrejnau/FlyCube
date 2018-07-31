#include "Context/VKView.h"

VKView::VKView(size_t binding, VkDescriptorType descriptor_type, VkDescriptorSet & descriptor_set)
    : m_binding(binding)
    , m_descriptor_type(descriptor_type)
    , m_descriptor_set(descriptor_set)
{
}

VKViewAllocatorByType::VKViewAllocatorByType(VKContext & context, VkDescriptorType descriptor_type)
    : m_context(context)
    , m_descriptor_type(descriptor_type)
{
}

VKView::Ptr VKViewAllocatorByType::Allocate(size_t count)
{
    if (m_offset + count > m_size)
    {
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    }
    m_offset += count;
    return std::make_shared<VKView>(m_offset - count, m_descriptor_type, m_descriptor_set);
}

void VKViewAllocatorByType::ResizeHeap(size_t req_size)
{
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = m_descriptor_type;
    pool_size.descriptorCount = req_size;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &pool_size;
    poolInfo.maxSets = 1;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(m_context.m_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (int i = 0; i < req_size; ++i)
    {
        bindings.emplace_back();
        auto& binding = bindings.back();
        binding.descriptorType = m_descriptor_type;
        binding.descriptorCount = 1;
        binding.binding = i;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = bindings.size();
    layout_info.pBindings = bindings.data();

    VkDescriptorSetLayout descriptor_set_layout;
    if (vkCreateDescriptorSetLayout(m_context.m_device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptor_set_layout;

    if (vkAllocateDescriptorSets(m_context.m_device, &allocInfo, &m_descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }
}

VKViewAllocator::VKViewAllocator(VKContext & context)
    : m_context(context)
{
}

VKView::Ptr VKViewAllocator::AllocateDescriptor(VkDescriptorType descriptor_type)
{
    auto& m_heap_alloc = GetAllocByType(descriptor_type);
    return m_heap_alloc.Allocate(1);
}

VKViewAllocatorByType & VKViewAllocator::GetAllocByType(VkDescriptorType type)
{
    auto it = m_allocators.find(type);
    it = m_allocators.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(m_context, type)).first;
    return it->second;
}
