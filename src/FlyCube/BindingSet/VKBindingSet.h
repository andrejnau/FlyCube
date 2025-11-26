#pragma once
#include "BindingSet/BindingSet.h"
#include "GPUDescriptorPool/VKGPUDescriptorPool.h"

#include <vulkan/vulkan.hpp>

#include <map>
#include <memory>
#include <vector>

class VKDevice;
class VKBindingSetLayout;

class VKBindingSet : public BindingSet {
public:
    VKBindingSet(VKDevice& device, const std::shared_ptr<VKBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;
    void WriteBindingsAndConstants(const std::vector<BindingDesc>& bindings,
                                   const std::vector<BindingConstantsData>& constants) override;

    const std::vector<vk::DescriptorSet>& GetDescriptorSets() const;

private:
    VKDevice& device_;
    std::vector<DescriptorSetPool> descriptors_;
    std::vector<vk::DescriptorSet> descriptor_sets_;
    std::shared_ptr<VKBindingSetLayout> layout_;
    std::shared_ptr<Resource> constants_fallback_buffer_;
    std::map<BindKey, uint64_t> constants_fallback_buffer_offsets_;
    std::map<BindKey, std::shared_ptr<View>> constants_fallback_buffer_views_;
};
