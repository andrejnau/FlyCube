#pragma once

#include <map>
#include <algorithm>
#include <Resource/VKResource.h>

class VKContext;

class VKDescriptorPool
{
public:
    VKDescriptorPool(VKContext& context);


    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout& set_layout, const std::map<VkDescriptorType, size_t>& count);

    void OnFrameBegin();

private:
    void ResizeHeap(const std::map<VkDescriptorType, size_t>& count);

private:
    VKContext& m_context;
};
