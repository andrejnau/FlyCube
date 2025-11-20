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
    std::map<uint32_t, vk::DescriptorType> bindless_type_;
    std::vector<vk::UniqueDescriptorSetLayout> descriptor_set_layouts_;
    std::vector<std::map<vk::DescriptorType, size_t>> descriptor_count_by_set_;
    vk::UniquePipelineLayout pipeline_layout_;
};

vk::DescriptorType GetDescriptorType(ViewType view_type);
