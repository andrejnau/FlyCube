#include "GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h"

#include "Device/VKDevice.h"

VKGPUBindlessDescriptorPoolTyped::VKGPUBindlessDescriptorPoolTyped(VKDevice& device, vk::DescriptorType type)
    : device_(device)
    , type_(type)
{
}

void VKGPUBindlessDescriptorPoolTyped::ResizeHeap(uint32_t req_size)
{
    req_size = std::min(req_size, device_.GetMaxDescriptorSetBindings(type_));

    if (size_ >= req_size) {
        return;
    }

    Descriptor descriptor;

    vk::DescriptorPoolSize pool_size = {};
    pool_size.type = type_;
    pool_size.descriptorCount = req_size;

    vk::DescriptorPoolCreateInfo pool_info = {};
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    descriptor.pool = device_.GetDevice().createDescriptorPoolUnique(pool_info);

    vk::DescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = type_;
    binding.descriptorCount = device_.GetMaxDescriptorSetBindings(binding.descriptorType);
    binding.stageFlags = vk::ShaderStageFlagBits::eAll;

    vk::DescriptorBindingFlags binding_flag = vk::DescriptorBindingFlagBits::eVariableDescriptorCount;

    vk::DescriptorSetLayoutBindingFlagsCreateInfo layout_flags_info = {};
    layout_flags_info.bindingCount = 1;
    layout_flags_info.pBindingFlags = &binding_flag;

    vk::DescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;
    layout_info.pNext = &layout_flags_info;

    descriptor.set_layout = device_.GetDevice().createDescriptorSetLayoutUnique(layout_info);

    vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_info = {};
    variable_descriptor_count_info.descriptorSetCount = 1;
    variable_descriptor_count_info.pDescriptorCounts = &req_size;

    vk::DescriptorSetAllocateInfo alloc_info = {};
    alloc_info.descriptorPool = descriptor.pool.get();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor.set_layout.get();
    alloc_info.pNext = &variable_descriptor_count_info;

    descriptor.set = std::move(device_.GetDevice().allocateDescriptorSetsUnique(alloc_info).front());

    if (size_) {
        vk::CopyDescriptorSet copy_descriptors;
        copy_descriptors.srcSet = descriptor_.set.get();
        copy_descriptors.dstSet = descriptor.set.get();
        copy_descriptors.descriptorCount = size_;
        device_.GetDevice().updateDescriptorSets(0, nullptr, 1, &copy_descriptors);
    }

    size_ = req_size;

    descriptor_.set.release();
    descriptor_ = std::move(descriptor);
}

VKGPUDescriptorPoolRange VKGPUBindlessDescriptorPoolTyped::Allocate(uint32_t count)
{
    auto it = empty_ranges_.lower_bound(count);
    if (it != empty_ranges_.end()) {
        size_t offset = it->second;
        size_t size = it->first;
        empty_ranges_.erase(it);
        return VKGPUDescriptorPoolRange(*this, offset, size);
    }
    if (offset_ + count > size_) {
        ResizeHeap(std::max(offset_ + count, 2 * (size_ + 1)));
    }
    offset_ += count;
    return VKGPUDescriptorPoolRange(*this, offset_ - count, count);
}

void VKGPUBindlessDescriptorPoolTyped::OnRangeDestroy(uint32_t offset, uint32_t size)
{
    empty_ranges_.emplace(size, offset);
}

vk::DescriptorSet VKGPUBindlessDescriptorPoolTyped::GetDescriptorSet() const
{
    return descriptor_.set.get();
}
