#pragma once
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <map>

class VKDevice;

struct DescriptorSetPool {
    vk::UniqueDescriptorPool pool;
    vk::UniqueDescriptorSet set;
};

class VKGPUDescriptorPool {
public:
    VKGPUDescriptorPool(VKDevice& device);
    DescriptorSetPool AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout,
                                            const std::map<vk::DescriptorType, size_t>& count);

private:
    vk::UniqueDescriptorPool CreateDescriptorPool(const std::map<vk::DescriptorType, size_t>& count);

    VKDevice& m_device;
};
