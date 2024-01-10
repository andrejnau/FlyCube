#include "Pipeline/VKGraphicsPipeline.h"

#include "Device/VKDevice.h"
#include "Program/ProgramBase.h"

#include <map>

vk::CompareOp Convert(ComparisonFunc func)
{
    switch (func) {
    case ComparisonFunc::kNever:
        return vk::CompareOp::eNever;
    case ComparisonFunc::kLess:
        return vk::CompareOp::eLess;
    case ComparisonFunc::kEqual:
        return vk::CompareOp::eEqual;
    case ComparisonFunc::kLessEqual:
        return vk::CompareOp::eLessOrEqual;
    case ComparisonFunc::kGreater:
        return vk::CompareOp::eGreater;
    case ComparisonFunc::kNotEqual:
        return vk::CompareOp::eNotEqual;
    case ComparisonFunc::kGreaterEqual:
        return vk::CompareOp::eGreaterOrEqual;
    case ComparisonFunc::kAlways:
        return vk::CompareOp::eAlways;
    default:
        assert(false);
        return vk::CompareOp::eLess;
    }
}

vk::StencilOp Convert(StencilOp op)
{
    switch (op) {
    case StencilOp::kKeep:
        return vk::StencilOp::eKeep;
    case StencilOp::kZero:
        return vk::StencilOp::eZero;
    case StencilOp::kReplace:
        return vk::StencilOp::eReplace;
    case StencilOp::kIncrSat:
        return vk::StencilOp::eIncrementAndClamp;
    case StencilOp::kDecrSat:
        return vk::StencilOp::eDecrementAndClamp;
    case StencilOp::kInvert:
        return vk::StencilOp::eInvert;
    case StencilOp::kIncr:
        return vk::StencilOp::eIncrementAndWrap;
    case StencilOp::kDecr:
        return vk::StencilOp::eDecrementAndWrap;
    default:
        assert(false);
        return vk::StencilOp::eKeep;
    }
}

vk::StencilOpState Convert(const StencilOpDesc& desc, uint8_t read_mask, uint8_t write_mask)
{
    vk::StencilOpState res = {};
    res.failOp = Convert(desc.fail_op);
    res.passOp = Convert(desc.pass_op);
    res.depthFailOp = Convert(desc.depth_fail_op);
    res.compareOp = Convert(desc.func);
    res.compareMask = read_mask;
    res.writeMask = write_mask;
    return res;
}

VKGraphicsPipeline::VKGraphicsPipeline(VKDevice& device, const GraphicsPipelineDesc& desc)
    : VKPipeline(device, desc.program, desc.layout)
    , m_desc(desc)
{
    if (desc.program->HasShader(ShaderType::kVertex)) {
        CreateInputLayout(m_binding_desc, m_attribute_desc);
    }

    const RenderPassDesc& render_pass_desc = m_desc.render_pass->GetDesc();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.vertexBindingDescriptionCount = m_binding_desc.size();
    vertex_input_info.pVertexBindingDescriptions = m_binding_desc.data();
    vertex_input_info.vertexAttributeDescriptionCount = m_attribute_desc.size();
    vertex_input_info.pVertexAttributeDescriptions = m_attribute_desc.data();

    vk::PipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = m_desc.rasterizer_desc.depth_bias != 0;
    rasterizer.depthBiasConstantFactor = m_desc.rasterizer_desc.depth_bias;
    switch (m_desc.rasterizer_desc.fill_mode) {
    case FillMode::kWireframe:
        rasterizer.polygonMode = vk::PolygonMode::eLine;
        break;
    case FillMode::kSolid:
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        break;
    }
    switch (m_desc.rasterizer_desc.cull_mode) {
    case CullMode::kNone:
        rasterizer.cullMode = vk::CullModeFlagBits::eNone;
        break;
    case CullMode::kFront:
        rasterizer.cullMode = vk::CullModeFlagBits::eFront;
        break;
    case CullMode::kBack:
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        break;
    }

    vk::PipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = m_desc.blend_desc.blend_enable;

    if (color_blend_attachment.blendEnable) {
        auto convert = [](Blend type) {
            switch (type) {
            case Blend::kZero:
                return vk::BlendFactor::eZero;
            case Blend::kSrcAlpha:
                return vk::BlendFactor::eSrcAlpha;
            case Blend::kInvSrcAlpha:
                return vk::BlendFactor::eOneMinusSrcAlpha;
            }
            throw std::runtime_error("unsupported");
        };

        auto convert_op = [](BlendOp type) {
            switch (type) {
            case BlendOp::kAdd:
                return vk::BlendOp::eAdd;
            }
            throw std::runtime_error("unsupported");
        };

        color_blend_attachment.srcColorBlendFactor = convert(m_desc.blend_desc.blend_src);
        color_blend_attachment.dstColorBlendFactor = convert(m_desc.blend_desc.blend_dest);
        color_blend_attachment.colorBlendOp = convert_op(m_desc.blend_desc.blend_op);
        color_blend_attachment.srcAlphaBlendFactor = convert(m_desc.blend_desc.blend_src_alpha);
        color_blend_attachment.dstAlphaBlendFactor = convert(m_desc.blend_desc.blend_dest_apha);
        color_blend_attachment.alphaBlendOp = convert_op(m_desc.blend_desc.blend_op_alpha);
    }

    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments(render_pass_desc.colors.size(),
                                                                               color_blend_attachment);

    vk::PipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = vk::LogicOp::eAnd;
    color_blending.attachmentCount = color_blend_attachments.size();
    color_blending.pAttachments = color_blend_attachments.data();

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.rasterizationSamples = static_cast<vk::SampleCountFlagBits>(render_pass_desc.sample_count);
    multisampling.sampleShadingEnable = multisampling.rasterizationSamples != vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.depthTestEnable = m_desc.depth_stencil_desc.depth_test_enable;
    depth_stencil.depthWriteEnable = m_desc.depth_stencil_desc.depth_write_enable;
    depth_stencil.depthCompareOp = Convert(m_desc.depth_stencil_desc.depth_func);
    depth_stencil.depthBoundsTestEnable = m_desc.depth_stencil_desc.depth_bounds_test_enable;
    depth_stencil.stencilTestEnable = m_desc.depth_stencil_desc.stencil_enable;
    depth_stencil.back = Convert(m_desc.depth_stencil_desc.back_face, m_desc.depth_stencil_desc.stencil_read_mask,
                                 m_desc.depth_stencil_desc.stencil_write_mask);
    depth_stencil.front = Convert(m_desc.depth_stencil_desc.front_face, m_desc.depth_stencil_desc.stencil_read_mask,
                                  m_desc.depth_stencil_desc.stencil_write_mask);

    std::vector<vk::DynamicState> dynamic_state_enables = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    if (m_device.IsVariableRateShadingSupported()) {
        dynamic_state_enables.emplace_back(vk::DynamicState::eFragmentShadingRateKHR);
    }

    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamic_state_enables.data();
    pipelineDynamicStateCreateInfo.dynamicStateCount = dynamic_state_enables.size();

    vk::GraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.stageCount = m_shader_stage_create_info.size();
    pipeline_info.pStages = m_shader_stage_create_info.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = GetRenderPass();
    pipeline_info.pDynamicState = &pipelineDynamicStateCreateInfo;

    m_pipeline = m_device.GetDevice().createGraphicsPipelineUnique({}, pipeline_info).value;
}

PipelineType VKGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

vk::RenderPass VKGraphicsPipeline::GetRenderPass() const
{
    return m_desc.render_pass->As<VKRenderPass>().GetRenderPass();
}

void VKGraphicsPipeline::CreateInputLayout(std::vector<vk::VertexInputBindingDescription>& m_binding_desc,
                                           std::vector<vk::VertexInputAttributeDescription>& m_attribute_desc)
{
    for (auto& vertex : m_desc.input) {
        decltype(auto) binding = m_binding_desc.emplace_back();
        decltype(auto) attribute = m_attribute_desc.emplace_back();
        attribute.location =
            m_desc.program->GetShader(ShaderType::kVertex)->GetInputLayoutLocation(vertex.semantic_name);
        attribute.binding = binding.binding = vertex.slot;
        binding.inputRate = vk::VertexInputRate::eVertex;
        binding.stride = vertex.stride;
        attribute.format = static_cast<vk::Format>(vertex.format);
    }
}
