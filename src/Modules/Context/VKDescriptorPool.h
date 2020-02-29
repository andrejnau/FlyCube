#pragma once

#include <map>
#include <algorithm>
#include <Resource/VKResource.h>

class VKContext;

class VKDescriptorPool
{
public:
    VKDescriptorPool(VKContext& context);

    vk::UniqueDescriptorSet AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout, const std::map<vk::DescriptorType, size_t>& count);

    void OnFrameBegin();

    vk::UniqueDescriptorPool m_descriptorPool;
private:
    void ResizeHeap(const std::map<vk::DescriptorType, size_t>& count);

private:
    VKContext& m_context;
};
