#pragma once

#include <map>
#include <algorithm>
#include <Context/VKContext.h>
#include <Context/VKResource.h>


class VKDescriptorHeapRange
{
public:
    VKDescriptorHeapRange(VKContext& context, size_t offset, VkDescriptorType descriptor_type, VkDescriptorSet descriptor_set)
        : m_context(context)
        , m_offset(offset)
        , m_descriptor_type(descriptor_type)
        , m_descriptor_set(descriptor_set)
    {
    }
    VKContext& m_context;
    size_t m_offset;
    VkDescriptorType m_descriptor_type;
    VkDescriptorSet m_descriptor_set;
};

class VKDescriptorHeapAllocator
{
public:
    
    VKDescriptorHeapAllocator(VKContext& context, VkDescriptorType descriptor_type)
        : m_context(context)
        , m_descriptor_type(descriptor_type)
    {
    }

    VKDescriptorHeapRange Allocate(size_t count)
    {
        if (m_offset + count > m_size)
        {
            ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
        }
        m_offset += count;
        return VKDescriptorHeapRange(m_context, m_offset - count, m_descriptor_type, m_descriptor_set);
    }

    void ResizeHeap(size_t req_size)
    {
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


        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptor_set_layout;

        if (vkAllocateDescriptorSets(m_context.m_device, &allocInfo, &m_descriptor_set) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }
    }

private:
    VKContext& m_context;
    size_t m_offset = 0;
    size_t m_size = 0;
    VkDescriptorType m_descriptor_type;
    VkDescriptorSet m_descriptor_set;
};


struct VkDescriptorByResource
{
    VKDescriptorHeapRange handle;
    bool exist;
};

class VKDescriptorManager
{
public:
    VKDescriptorManager(VKContext& context)
        : m_context(context)
    {
    }

    VKDescriptorHeapAllocator& GetAllocByType(VkDescriptorType type)
    {
        auto it = m_descriptors_alloc.find(type);
        if (it == m_descriptors_alloc.end())
        {
            it = m_descriptors_alloc.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(m_context, type)).first;
        }
        return it->second;

  
    }

    VkDescriptorByResource GetDescriptor(const VKBindKey& bind_key, VKResource& res)
    {
        auto& m_heap_alloc = GetAllocByType(std::get<VkDescriptorType>(bind_key));

        bool exist = true;
        auto it = res.descriptors.find(bind_key);
        if (it == res.descriptors.end())
        {
            exist = false;
            it = res.descriptors.emplace(std::piecewise_construct,
                std::forward_as_tuple(bind_key),
                std::forward_as_tuple(m_heap_alloc.Allocate(1))).first;
        }
        return { it->second, exist };
    }

    std::map<VkDescriptorType, VKDescriptorHeapAllocator> m_descriptors_alloc;
private:
    VKContext& m_context;
};