#pragma once
#include "PipelineState.h"
#include <Instance/BaseTypes.h>
#include <Utilities/Vulkan.h>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKDevice;

class VKPipelineState : public PipelineState
{
public:
    VKPipelineState(VKDevice& device, const PipelineStateDesc& desc);

private:
    void VKPipelineState::CreateInputLayout(const std::vector<uint32_t>& spirv_binary,
                                            std::vector<vk::VertexInputBindingDescription>& binding_desc,
                                            std::vector<vk::VertexInputAttributeDescription>& attribute_desc);
    void CreateGrPipeLine();
    void CreateComputePipeLine();

    VKDevice& m_device;
    PipelineStateDesc m_desc;
    std::vector<vk::VertexInputBindingDescription> m_binding_desc;
    std::vector<vk::VertexInputAttributeDescription> m_attribute_desc;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_info;
    std::map<ShaderType, vk::UniqueShaderModule> m_shader_modules;
    vk::UniquePipeline m_pipeline;
    RasterizerDesc m_rasterizer_desc = {};
    BlendDesc m_blend_desc = {};
    DepthStencilDesc m_depth_stencil_desc = {};
    size_t m_msaa_count = 1;
    bool m_is_compute = false;
    std::vector<vk::AttachmentDescription> m_attachment_descriptions;
    std::vector<vk::AttachmentReference> m_attachment_references;
    vk::UniqueRenderPass m_render_pass;
    std::map<ShaderType, spirv_cross::SmallVector<spirv_cross::EntryPoint>> m_entries;
};
