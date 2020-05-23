#include "BindingSet/VKBindingSet.h"

VKBindingSet::VKBindingSet(const std::vector<vk::DescriptorSet>& descriptor_sets, vk::PipelineLayout pipeline_layout)
    : m_descriptor_sets(descriptor_sets)
    , m_pipeline_layout(pipeline_layout)
{
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return m_descriptor_sets;
}

vk::PipelineLayout VKBindingSet::GetPipelineLayout() const
{
    return m_pipeline_layout;
}
