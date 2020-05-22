#pragma once
#include <map>
#include <algorithm>
#include <Utilities/Vulkan.h>

class VKDevice;

class VKGPUDescriptorPool
{
public:
    VKGPUDescriptorPool(VKDevice& device);
    vk::UniqueDescriptorSet AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout, const std::map<vk::DescriptorType, size_t>& count);

private:
    void ResizeHeap(const std::map<vk::DescriptorType, size_t>& count);

    VKDevice& m_device;
    vk::UniqueDescriptorPool m_descriptorPool;
};
