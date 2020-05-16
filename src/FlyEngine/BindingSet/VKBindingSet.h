#pragma once
#include "BindingSet/BindingSet.h"
#include <Utilities/Vulkan.h>

class VKBindingSet
    : public BindingSet
{
public:
    VKBindingSet(const std::vector<vk::DescriptorSet>& descriptor_sets, vk::PipelineLayout pipeline_layout);
    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::PipelineLayout m_pipeline_layout;
};
