#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

#include <vulkan/vulkan.hpp>

#include <map>
#include <set>
#include <vector>

class VKDevice;

struct AllocateDescriptorSetDesc {
    std::map<vk::DescriptorType, size_t> count;
    size_t inline_uniform_block_bindings = 0;
};

class VKBindingSetLayout : public BindingSetLayout {
public:
    VKBindingSetLayout(VKDevice& device,
                       const std::vector<BindKey>& bind_keys,
                       const std::vector<BindingConstants>& constants);

    const std::map<uint32_t, vk::DescriptorType>& GetBindlessType() const;
    const std::vector<vk::UniqueDescriptorSetLayout>& GetDescriptorSetLayouts() const;
    const std::vector<AllocateDescriptorSetDesc>& GetAllocateDescriptorSetDescs() const;
    const std::set<BindKey>& GetInlineUniformBlocks() const;
    const std::vector<BindingConstants>& GetFallbackConstants() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    std::map<uint32_t, vk::DescriptorType> bindless_type_;
    std::vector<vk::UniqueDescriptorSetLayout> descriptor_set_layouts_;
    std::vector<AllocateDescriptorSetDesc> allocate_descriptor_set_descs_;
    std::set<BindKey> inline_uniform_blocks_;
    std::vector<BindingConstants> fallback_constants_;
    vk::UniquePipelineLayout pipeline_layout_;
};

vk::DescriptorType GetDescriptorType(ViewType view_type);
