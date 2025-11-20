#pragma once
#include "Pipeline/Pipeline.h"

#include <vulkan/vulkan.hpp>

#include <deque>

class VKDevice;

class VKPipeline : public Pipeline {
public:
    VKPipeline(VKDevice& device,
               const std::vector<std::shared_ptr<Shader>>& shaders,
               const std::shared_ptr<BindingSetLayout>& layout);
    vk::PipelineLayout GetPipelineLayout() const;
    vk::Pipeline GetPipeline() const;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;

protected:
    VKDevice& device_;
    std::deque<std::string> entry_point_names;
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_info_;
    std::vector<vk::UniqueShaderModule> shader_modules_;
    vk::UniquePipeline pipeline_;
    vk::PipelineLayout pipeline_layout_;
    std::map<uint64_t, uint32_t> shader_ids_;
};
