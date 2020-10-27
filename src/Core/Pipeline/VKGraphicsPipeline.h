#pragma once
#include "Pipeline.h"
#include <Instance/BaseTypes.h>
#include <Utilities/Vulkan.h>
#include <RenderPass/VKRenderPass.h>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

vk::ShaderStageFlagBits ExecutionModel2Bit(spv::ExecutionModel model);

class VKDevice;

class VKGraphicsPipeline : public Pipeline
{
public:
    VKGraphicsPipeline(VKDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;

    vk::Pipeline GetPipeline() const;
    vk::RenderPass GetRenderPass() const;

private:
    void VKGraphicsPipeline::CreateInputLayout(const std::vector<uint32_t>& spirv_binary,
                                       std::vector<vk::VertexInputBindingDescription>& binding_desc,
                                       std::vector<vk::VertexInputAttributeDescription>& attribute_desc);
    void CreateGrPipeLine();

    VKDevice& m_device;
    vk::UniquePipeline m_pipeline;
    GraphicsPipelineDesc m_desc;
    std::vector<vk::VertexInputBindingDescription> m_binding_desc;
    std::vector<vk::VertexInputAttributeDescription> m_attribute_desc;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_info;
    std::map<ShaderType, vk::UniqueShaderModule> m_shader_modules;
    std::map<ShaderType, spirv_cross::SmallVector<spirv_cross::EntryPoint>> m_entries;
};
