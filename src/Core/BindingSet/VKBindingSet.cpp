#include "BindingSet/VKBindingSet.h"

VKBindingSet::VKBindingSet(std::vector<vk::DescriptorSet>&& descriptor_sets, std::vector<vk::UniqueDescriptorSet>&& descriptor_sets_unique, vk::PipelineLayout pipeline_layout)
    : m_descriptor_sets(std::move(descriptor_sets_unique))
    , m_descriptor_sets_raw(descriptor_sets)
    , m_pipeline_layout(pipeline_layout)
{
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return m_descriptor_sets_raw;
}

vk::PipelineLayout VKBindingSet::GetPipelineLayout() const
{
    return m_pipeline_layout;
}
