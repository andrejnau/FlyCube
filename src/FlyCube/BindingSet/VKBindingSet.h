#pragma once
#include "BindingSet/BindingSetBase.h"
#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

class VKDevice;
class VKBindingSetLayout;

class VKBindingSet : public BindingSetBase {
public:
    VKBindingSet(VKDevice& device, const std::shared_ptr<VKBindingSetLayout>& layout);

    void WriteBindings(const WriteBindingsDesc& desc) override;

    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;

private:
    void WriteDescriptor(std::vector<vk::WriteDescriptorSet>& descriptors, const BindingDesc& binding);

    VKDevice& device_;
    std::vector<DescriptorSetPool> descriptors_;
    std::vector<vk::DescriptorSet> descriptor_sets_;
    std::shared_ptr<VKBindingSetLayout> layout_;
};
