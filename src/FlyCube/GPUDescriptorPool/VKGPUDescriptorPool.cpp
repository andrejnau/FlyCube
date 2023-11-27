#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include "Device/VKDevice.h"

VKGPUDescriptorPool::VKGPUDescriptorPool(VKDevice& device)
    : m_device(device)
{
}

vk::UniqueDescriptorPool VKGPUDescriptorPool::CreateDescriptorPool(const std::map<vk::DescriptorType, size_t>& count)
{
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    for (auto& x : count) {
        pool_sizes.emplace_back();
        vk::DescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = x.first;
        pool_size.descriptorCount = x.second;
    }

    // TODO: fix me
    if (count.empty()) {
        pool_sizes.emplace_back();
        vk::DescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = vk::DescriptorType::eSampler;
        pool_size.descriptorCount = 1;
    }

    vk::DescriptorPoolCreateInfo pool_info = {};
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    return m_device.GetDevice().createDescriptorPoolUnique(pool_info);
}

DescriptorSetPool VKGPUDescriptorPool::AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout,
                                                             const std::map<vk::DescriptorType, size_t>& count)
{
    DescriptorSetPool res = {};
    res.pool = CreateDescriptorPool(count);

    vk::DescriptorSetAllocateInfo alloc_info = {};
    alloc_info.descriptorPool = res.pool.get();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &set_layout;
    auto descriptor_sets = m_device.GetDevice().allocateDescriptorSetsUnique(alloc_info);
    res.set = std::move(descriptor_sets.front());

    return res;
}
