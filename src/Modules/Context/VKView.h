#pragma once

#include <map>
#include <algorithm>
#include <stdint.h>
#include <Context/VKResource.h>
#include <Context/View.h>

#include <Context/VKContext.h>
#include <vulkan/vulkan.h>

class VKView : public View
{
public:
    VKView(size_t binding, VkDescriptorType descriptor_type, VkDescriptorSet& descriptor_set);

    size_t m_binding;
    VkDescriptorType m_descriptor_type;
    std::reference_wrapper<VkDescriptorSet> m_descriptor_set;
};

class VKViewAllocatorByType
{
public:
    VKViewAllocatorByType(VKContext& context, VkDescriptorType descriptor_type);

    VKView::Ptr Allocate(size_t count);

private:
    void ResizeHeap(size_t req_size);

private:
    VKContext& m_context;
    size_t m_offset = 0;
    size_t m_size = 0;
    VkDescriptorType m_descriptor_type;
    VkDescriptorSet m_descriptor_set;
};

class VKViewAllocator
{
public:
    VKViewAllocator(VKContext& context);

    VKView::Ptr AllocateDescriptor(VkDescriptorType descriptor_type);

private:
    VKViewAllocatorByType & GetAllocByType(VkDescriptorType type);

    VKContext& m_context;
    std::map<VkDescriptorType, VKViewAllocatorByType> m_allocators;
};
