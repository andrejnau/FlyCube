#include "BindingSet/VKBindingSet.h"
#include <Device/VKDevice.h>
#include <BindingSetLayout/VKBindingSetLayout.h>
#include <View/VKView.h>

VKBindingSet::VKBindingSet(VKDevice& device, const std::shared_ptr<VKBindingSetLayout>& layout)
    : m_device(device)
    , m_layout(layout)
{
    decltype(auto) bindless_type = m_layout->GetBindlessType();
    decltype(auto) descriptor_set_layouts = m_layout->GetDescriptorSetLayouts();
    decltype(auto) descriptor_count_by_set = m_layout->GetDescriptorCountBySet();
    for (size_t i = 0; i < descriptor_set_layouts.size(); ++i)
    {
        if (bindless_type.count(i))
        {
            m_descriptor_sets.emplace_back(m_device.GetGPUBindlessDescriptorPool(bindless_type.at(i)).GetDescriptorSet());
        }
        else
        {
            m_descriptors.emplace_back(m_device.GetGPUDescriptorPool().AllocateDescriptorSet(descriptor_set_layouts[i].get(), descriptor_count_by_set[i]));
            m_descriptor_sets.emplace_back(m_descriptors.back().set.get());
        }
    }
}

void VKBindingSet::WriteBindings(const std::vector<BindingDesc>& bindings)
{
    decltype(auto) bindless_type = m_layout->GetBindlessType();
    std::vector<vk::WriteDescriptorSet> descriptor_writes;
    std::list<vk::DescriptorImageInfo> list_image_info;
    std::list<vk::DescriptorBufferInfo> list_buffer_info;
    std::list<vk::WriteDescriptorSetAccelerationStructureKHR> list_as;

    for (const auto& binding : bindings)
    {
        bool is_rtv_dsv = false;
        switch (binding.bind_key.view_type)
        {
        case ViewType::kRenderTarget:
        case ViewType::kDepthStencil:
            is_rtv_dsv = true;
            break;
        }

        if (is_rtv_dsv || !binding.view)
        {
            continue;
        }

        ShaderType shader_type = binding.bind_key.shader_type;
        if (bindless_type.count(static_cast<size_t>(shader_type)))
        {
            continue;
        }

        vk::WriteDescriptorSet descriptor_write = {};
        descriptor_write.dstSet = m_descriptor_sets[static_cast<size_t>(shader_type)];
        descriptor_write.dstBinding = binding.bind_key.slot;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = GetDescriptorType(binding.bind_key.view_type);
        descriptor_write.descriptorCount = 1;

        decltype(auto) vk_view = binding.view->As<VKView>();
        vk_view.WriteView(descriptor_write, list_image_info, list_buffer_info, list_as);

        if (descriptor_write.pImageInfo || descriptor_write.pBufferInfo || descriptor_write.pNext)
        {
            descriptor_writes.push_back(descriptor_write);
        }
    }

    if (!descriptor_writes.empty())
    {
        m_device.GetDevice().updateDescriptorSets(descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return m_descriptor_sets;
}
