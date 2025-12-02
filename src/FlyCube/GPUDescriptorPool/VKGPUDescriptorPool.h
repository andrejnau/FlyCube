#pragma once
#include "BindingSetLayout/VKBindingSetLayout.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

struct DescriptorSetPool {
    vk::UniqueDescriptorPool pool;
    vk::DescriptorSet set;
};

class VKGPUDescriptorPool {
public:
    explicit VKGPUDescriptorPool(VKDevice& device);
    DescriptorSetPool AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout,
                                            const AllocateDescriptorSetDesc& desc);

private:
    vk::UniqueDescriptorPool CreateDescriptorPool(const AllocateDescriptorSetDesc& desc);

    VKDevice& device_;
};
