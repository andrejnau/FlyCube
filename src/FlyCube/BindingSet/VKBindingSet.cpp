#include "BindingSet/VKBindingSet.h"

#include "BindingSetLayout/VKBindingSetLayout.h"
#include "Device/VKDevice.h"
#include "View/VKView.h"

#include <deque>

VKBindingSet::VKBindingSet(VKDevice& device, const std::shared_ptr<VKBindingSetLayout>& layout)
    : device_(device)
    , layout_(layout)
{
    decltype(auto) bindless_type = layout_->GetBindlessType();
    decltype(auto) descriptor_set_layouts = layout_->GetDescriptorSetLayouts();
    decltype(auto) allocate_descriptor_set_descs = layout_->GetAllocateDescriptorSetDescs();
    for (size_t i = 0; i < descriptor_set_layouts.size(); ++i) {
        if (bindless_type.contains(i)) {
            descriptor_sets_.emplace_back(device_.GetGPUBindlessDescriptorPool(bindless_type.at(i)).GetDescriptorSet());
        } else {
            descriptors_.emplace_back(device_.GetGPUDescriptorPool().AllocateDescriptorSet(
                descriptor_set_layouts[i].get(), allocate_descriptor_set_descs[i]));
            descriptor_sets_.emplace_back(descriptors_.back().set.get());
        }
    }

    CreateConstantsFallbackBuffer(device_, layout_->GetFallbackConstants());
    std::vector<vk::WriteDescriptorSet> descriptors;
    for (const auto& [bind_key, view] : fallback_constants_buffer_views_) {
        assert(bind_key.count == 1);
        WriteDescriptor(descriptors, { bind_key, view });
    }
    if (!descriptors.empty()) {
        device_.GetDevice().updateDescriptorSets(descriptors.size(), descriptors.data(), 0, nullptr);
    }
}

void VKBindingSet::WriteBindings(const WriteBindingsDesc& desc)
{
    std::vector<vk::WriteDescriptorSet> descriptors;
    for (const auto& binding : desc.bindings) {
        WriteDescriptor(descriptors, binding);
    }

    std::deque<vk::WriteDescriptorSetInlineUniformBlock> write_descriptor_set_inline_uniform_blocks;
    for (const auto& [bind_key, data] : desc.constants) {
        if (layout_->GetInlineUniformBlocks().contains(bind_key)) {
            auto& write_descriptor_set_inline_uniform_block = write_descriptor_set_inline_uniform_blocks.emplace_back();
            write_descriptor_set_inline_uniform_block.dataSize = data.size();
            write_descriptor_set_inline_uniform_block.pData = data.data();

            vk::WriteDescriptorSet descriptor = {};
            descriptor.descriptorType = vk::DescriptorType::eInlineUniformBlock;
            descriptor.dstSet = descriptor_sets_[bind_key.space];
            descriptor.dstBinding = bind_key.slot;
            descriptor.descriptorCount = data.size();
            descriptor.pNext = &write_descriptor_set_inline_uniform_block;
            descriptors.emplace_back(descriptor);
        } else {
            fallback_constants_buffer_->UpdateUploadBuffer(fallback_constants_buffer_offsets_.at(bind_key), data.data(),
                                                           data.size());
        }
    }

    if (!descriptors.empty()) {
        device_.GetDevice().updateDescriptorSets(descriptors.size(), descriptors.data(), 0, nullptr);
    }
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return descriptor_sets_;
}

void VKBindingSet::WriteDescriptor(std::vector<vk::WriteDescriptorSet>& descriptors, const BindingDesc& binding)
{
    decltype(auto) vk_view = binding.view->As<VKView>();
    vk::WriteDescriptorSet descriptor = vk_view.GetDescriptor();
    descriptor.descriptorType = GetDescriptorType(binding.bind_key.view_type);
    descriptor.dstSet = descriptor_sets_[binding.bind_key.space];
    descriptor.dstBinding = binding.bind_key.slot;
    descriptor.dstArrayElement = 0;
    descriptor.descriptorCount = 1;
    if (descriptor.pImageInfo || descriptor.pBufferInfo || descriptor.pTexelBufferView || descriptor.pNext) {
        descriptors.emplace_back(descriptor);
    }
}
