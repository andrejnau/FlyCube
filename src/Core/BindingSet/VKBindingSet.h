#pragma once
#include "BindingSet/BindingSet.h"
#include <Utilities/Vulkan.h>

class VKBindingSet
    : public BindingSet
{
public:
    VKBindingSet(std::vector<vk::DescriptorSet>&& descriptor_sets, std::vector<vk::UniqueDescriptorSet>&& descriptor_sets_unique, vk::PipelineLayout pipeline_layout);
    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    std::vector<vk::UniqueDescriptorSet> m_descriptor_sets;
    std::vector<vk::DescriptorSet> m_descriptor_sets_raw;
    vk::PipelineLayout m_pipeline_layout;
};
