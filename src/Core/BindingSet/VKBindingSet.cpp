#include "BindingSet/VKBindingSet.h"

VKBindingSet::VKBindingSet(bool is_compute, const std::vector<vk::DescriptorSet>& descriptor_sets, vk::PipelineLayout pipeline_layout)
    : m_is_compute(is_compute)
    , m_descriptor_sets(descriptor_sets)
    , m_pipeline_layout(pipeline_layout)
{
}

bool VKBindingSet::IsCompute() const
{
    return m_is_compute;
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return m_descriptor_sets;
}

vk::PipelineLayout VKBindingSet::GetPipelineLayout() const
{
    return m_pipeline_layout;
}
