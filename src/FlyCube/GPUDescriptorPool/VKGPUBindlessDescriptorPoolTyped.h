#pragma once
#include "GPUDescriptorPool/VKGPUDescriptorPoolRange.h"

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <map>

class VKDevice;

class VKGPUBindlessDescriptorPoolTyped {
public:
    VKGPUBindlessDescriptorPoolTyped(VKDevice& device, vk::DescriptorType type);
    VKGPUDescriptorPoolRange Allocate(uint32_t count);
    void OnRangeDestroy(uint32_t offset, uint32_t size);
    vk::DescriptorSet GetDescriptorSet() const;

private:
    void ResizeHeap(uint32_t req_size);

    VKDevice& device_;
    vk::DescriptorType type_;
    uint32_t size_ = 0;
    uint32_t offset_ = 0;
    struct Descriptor {
        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSetLayout set_layout;
        vk::UniqueDescriptorSet set;
    } descriptor_;
    std::multimap<uint32_t, uint32_t> empty_ranges_;
};
