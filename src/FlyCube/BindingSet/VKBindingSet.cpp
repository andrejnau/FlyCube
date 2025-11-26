#include "BindingSet/VKBindingSet.h"

#include "BindingSetLayout/VKBindingSetLayout.h"
#include "Device/VKDevice.h"
#include "View/VKView.h"

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

    if (!device_.IsInlineUniformBlockSupported()) {
        CreateConstantsFallbackBuffer(device_, layout_->GetConstants());
    }
}

void VKBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    WriteBindingsAndConstants(bindings, {});
}

void VKBindingSet::WriteBindingsAndConstants(const std::vector<BindingDesc>& bindings,
                                             const std::vector<BindingConstantsData>& constants)
{
    std::vector<vk::WriteDescriptorSet> descriptors;
    auto add_descriptor = [&](const BindingDesc& binding) {
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
    };

    for (const auto& binding : bindings) {
        add_descriptor(binding);
    }

    if (device_.IsInlineUniformBlockSupported()) {
        for (const auto& [bind_key, data] : constants) {
            vk::WriteDescriptorSetInlineUniformBlock write_descriptor_set_inline_uniform_block = {};
            write_descriptor_set_inline_uniform_block.dataSize = data.size();
            write_descriptor_set_inline_uniform_block.pData = data.data();

            vk::WriteDescriptorSet descriptor = {};
            descriptor.descriptorType = vk::DescriptorType::eInlineUniformBlock;
            descriptor.dstSet = descriptor_sets_[bind_key.space];
            descriptor.dstBinding = bind_key.slot;
            descriptor.descriptorCount = data.size();
            descriptor.pNext = &write_descriptor_set_inline_uniform_block;
            descriptors.emplace_back(descriptor);
        }
    } else {
        for (const auto& [bind_key, view] : fallback_constants_buffer_views_) {
            assert(bind_key.count == 1);
            add_descriptor({ bind_key, view });
        }
        UpdateConstantsFallbackBuffer(constants);
    }

    if (!descriptors.empty()) {
        device_.GetDevice().updateDescriptorSets(descriptors.size(), descriptors.data(), 0, nullptr);
    }
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return descriptor_sets_;
}
