#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include "Device/VKDevice.h"

VKGPUDescriptorPool::VKGPUDescriptorPool(VKDevice& device)
    : device_(device)
{
}

vk::UniqueDescriptorPool VKGPUDescriptorPool::CreateDescriptorPool(const AllocateDescriptorSetDesc& desc)
{
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    for (const auto& [type, count] : desc.count) {
        pool_sizes.emplace_back();
        vk::DescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = type;
        pool_size.descriptorCount = count;
    }

    // TODO: fix me
    if (pool_sizes.empty()) {
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

    vk::DescriptorPoolInlineUniformBlockCreateInfo descriptor_pool_inline_uniform_block_info = {};
    descriptor_pool_inline_uniform_block_info.maxInlineUniformBlockBindings = desc.inline_uniform_block_bindings;
    if (descriptor_pool_inline_uniform_block_info.maxInlineUniformBlockBindings > 0) {
        pool_info.pNext = &descriptor_pool_inline_uniform_block_info;
    }

    return device_.GetDevice().createDescriptorPoolUnique(pool_info);
}

DescriptorSetPool VKGPUDescriptorPool::AllocateDescriptorSet(const vk::DescriptorSetLayout& set_layout,
                                                             const AllocateDescriptorSetDesc& desc)
{
    DescriptorSetPool res = {};
    res.pool = CreateDescriptorPool(desc);

    vk::DescriptorSetAllocateInfo alloc_info = {};
    alloc_info.descriptorPool = res.pool.get();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &set_layout;
    auto descriptor_sets = device_.GetDevice().allocateDescriptorSetsUnique(alloc_info);
    res.set = std::move(descriptor_sets.front());

    return res;
}
