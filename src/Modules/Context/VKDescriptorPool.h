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

    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout& set_layout, size_t prog_id);

    void ReqFrameDescriptionDrawCalls(size_t count, size_t prog_id);

    void ReqFrameDescription(VkDescriptorType type, size_t count);

    void OnFrameBegin();

    std::map<VkDescriptorType, size_t> m_size_by_type;

    size_t m_max_descriptor_sets = 0;
    std::map<size_t, size_t> m_max_descriptor_sets_by_prog;
    std::map<size_t, size_t> m_cur_descriptor_sets_by_prog;

    bool first_frame_alloc = true;
private:
    VKContext& m_context;
};
