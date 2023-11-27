#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKBindingSetLayout : public BindingSetLayout {
public:
    VKBindingSetLayout(VKDevice& device, const std::vector<BindKey>& descs);

    const std::map<uint32_t, vk::DescriptorType>& GetBindlessType() const;
    const std::vector<vk::UniqueDescriptorSetLayout>& GetDescriptorSetLayouts() const;
    const std::vector<std::map<vk::DescriptorType, size_t>>& GetDescriptorCountBySet() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    std::map<uint32_t, vk::DescriptorType> m_bindless_type;
    std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts;
    std::vector<std::map<vk::DescriptorType, size_t>> m_descriptor_count_by_set;
    vk::UniquePipelineLayout m_pipeline_layout;
};

vk::DescriptorType GetDescriptorType(ViewType view_type);
