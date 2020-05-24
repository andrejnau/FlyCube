#include "BindingSet/VKBindingSet.h"

VKBindingSet::VKBindingSet(std::vector<vk::UniqueDescriptorSet>&& descriptor_sets, vk::PipelineLayout pipeline_layout)
    : m_descriptor_sets(std::move(descriptor_sets))
    , m_pipeline_layout(pipeline_layout)
{
    for (auto& x : m_descriptor_sets)
    {
        m_descriptor_sets_raw.emplace_back(x.get());
    }
}

const std::vector<vk::DescriptorSet>& VKBindingSet::GetDescriptorSets() const
{
    return m_descriptor_sets_raw;
}

vk::PipelineLayout VKBindingSet::GetPipelineLayout() const
{
    return m_pipeline_layout;
}
