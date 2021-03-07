#pragma once
#include "Pipeline/VKPipeline.h"
#include <Instance/BaseTypes.h>
#include <vulkan/vulkan.hpp>
#include <RenderPass/VKRenderPass.h>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKDevice;

class VKComputePipeline : public VKPipeline
{
public:
    VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    vk::Pipeline GetPipeline() const override;
    vk::PipelineLayout GetPipelineLayout() const override;
    vk::PipelineBindPoint GetPipelineBindPoint() const override;

private:
    void CreateComputePipeLine();

    VKDevice& m_device;
    vk::UniquePipeline m_pipeline;
    ComputePipelineDesc m_desc;
    std::vector<vk::VertexInputBindingDescription> m_binding_desc;
    std::vector<vk::VertexInputAttributeDescription> m_attribute_desc;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_info;
    std::map<ShaderType, vk::UniqueShaderModule> m_shader_modules;
    std::map<ShaderType, spirv_cross::SmallVector<spirv_cross::EntryPoint>> m_entries;
};
