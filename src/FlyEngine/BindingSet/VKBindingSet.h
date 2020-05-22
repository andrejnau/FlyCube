#pragma once
#include "BindingSet/BindingSet.h"
#include <Utilities/Vulkan.h>

class VKBindingSet
    : public BindingSet
{
public:
    VKBindingSet(bool is_compute, const std::vector<vk::DescriptorSet>& descriptor_sets, vk::PipelineLayout pipeline_layout);
    bool IsCompute() const;
    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    bool m_is_compute;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::PipelineLayout m_pipeline_layout;
};
