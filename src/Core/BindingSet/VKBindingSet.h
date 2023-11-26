#pragma once
#include "BindingSet/BindingSet.h"
#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include <vulkan/vulkan.hpp>

class VKDevice;
class VKBindingSetLayout;

class VKBindingSet : public BindingSet {
public:
    VKBindingSet(VKDevice& device, const std::shared_ptr<VKBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;
    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;

private:
    VKDevice& m_device;
    std::vector<DescriptorSetPool> m_descriptors;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    std::shared_ptr<VKBindingSetLayout> m_layout;
};
