#pragma once
#include "BindingSet/BindingSet.h"
#include <vulkan/vulkan.hpp>

class VKDevice;
class VKBindingSetLayout;

class VKBindingSet
    : public BindingSet
{
public:
    VKBindingSet(VKDevice& device, const std::shared_ptr<VKBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;
    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    VKDevice& m_device;
    std::vector<vk::UniqueDescriptorSet> m_descriptor_sets_unique;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    std::shared_ptr<VKBindingSetLayout> m_layout;
};
