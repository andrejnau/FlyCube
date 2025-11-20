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
    decltype(auto) descriptor_count_by_set = layout_->GetDescriptorCountBySet();
    for (size_t i = 0; i < descriptor_set_layouts.size(); ++i) {
        if (bindless_type.contains(i)) {
            descriptor_sets_.emplace_back(device_.GetGPUBindlessDescriptorPool(bindless_type.at(i)).GetDescriptorSet());
        } else {
            descriptors_.emplace_back(device_.GetGPUDescriptorPool().AllocateDescriptorSet(
                descriptor_set_layouts[i].get(), descriptor_count_by_set[i]));
            descriptor_sets_.emplace_back(descriptors_.back().set.get());
        }
    }
}

void VKBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    std::vector<vk::WriteDescriptorSet> descriptors;
    for (const auto& binding : bindings) {
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

    if (!descriptors.empty()) {
        device_.GetDevice().updateDescriptorSets(descriptors.size(), descriptors.data(), 0, nullptr);
    }
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return descriptor_sets_;
}
