#pragma once

#include <map>
#include <algorithm>
#include <Context/VKResource.h>

class VKContext;

class VKDescriptorPool
{
public:
    VKDescriptorPool(VKContext& context);

    void ResizeHeap();

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout& set_layout);

    void ReqFrameDescriptionDrawCalls(size_t count);

    void ReqFrameDescription(VkDescriptorType type, size_t count);

    void OnFrameBegin();

    std::map<VkDescriptorType, size_t> m_size_by_type;

    size_t m_max_descriptor_sets = 0;

    bool first_frame_alloc = true;
private:
    VKContext& m_context;
};
