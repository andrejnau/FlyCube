#include "Pipeline/VKGraphicsPipeline.h"

#include "Device/VKDevice.h"
#include "Program/ProgramBase.h"
#include "Utilities/NotReached.h"

#include <map>

namespace {

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
        NOTREACHED();
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
        NOTREACHED();
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

vk::UniqueRenderPass CreateRenderPass(VKDevice& device, const RenderPassDesc& desc)
{
    std::vector<vk::AttachmentDescription2> attachment_descriptions;
    auto add_attachment = [&](vk::AttachmentReference2& reference, gli::format format, vk::ImageLayout layout,
                              RenderPassLoadOp load_op, RenderPassStoreOp store_op) {
        if (format == gli::FORMAT_UNDEFINED) {
            reference.attachment = VK_ATTACHMENT_UNUSED;
            return;
        }
        vk::AttachmentDescription2& description = attachment_descriptions.emplace_back();
        description.format = static_cast<vk::Format>(format);
        description.samples = static_cast<vk::SampleCountFlagBits>(desc.sample_count);
        description.loadOp = ConvertRenderPassLoadOp(load_op);
        description.storeOp = ConvertRenderPassStoreOp(store_op);
        description.initialLayout = layout;
        description.finalLayout = layout;

        reference.attachment = attachment_descriptions.size() - 1;
        reference.layout = layout;
    };

    vk::SubpassDescription2 sub_pass = {};
    sub_pass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

    std::vector<vk::AttachmentReference2> color_attachment_references;
    for (auto& rtv : desc.colors) {
        add_attachment(color_attachment_references.emplace_back(), rtv.format, vk::ImageLayout::eColorAttachmentOptimal,
                       rtv.load_op, rtv.store_op);
    }

    sub_pass.colorAttachmentCount = color_attachment_references.size();
    sub_pass.pColorAttachments = color_attachment_references.data();

    vk::AttachmentReference2 depth_attachment_reference = {};
    if (desc.depth_stencil.format != gli::FORMAT_UNDEFINED) {
        add_attachment(depth_attachment_reference, desc.depth_stencil.format,
                       vk::ImageLayout::eDepthStencilAttachmentOptimal, desc.depth_stencil.depth_load_op,
                       desc.depth_stencil.depth_store_op);
        if (depth_attachment_reference.attachment != VK_ATTACHMENT_UNUSED) {
            vk::AttachmentDescription2& description = attachment_descriptions[depth_attachment_reference.attachment];
            description.stencilLoadOp = ConvertRenderPassLoadOp(desc.depth_stencil.stencil_load_op);
            description.stencilStoreOp = ConvertRenderPassStoreOp(desc.depth_stencil.stencil_store_op);
        }
        sub_pass.pDepthStencilAttachment = &depth_attachment_reference;
    }

    vk::RenderPassCreateInfo2 render_pass_info = {};
    render_pass_info.attachmentCount = attachment_descriptions.size();
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &sub_pass;
    return device.GetDevice().createRenderPass2Unique(render_pass_info);
}

} // namespace

vk::AttachmentLoadOp ConvertRenderPassLoadOp(RenderPassLoadOp op)
{
    switch (op) {
    case RenderPassLoadOp::kLoad:
        return vk::AttachmentLoadOp::eLoad;
    case RenderPassLoadOp::kClear:
        return vk::AttachmentLoadOp::eClear;
    case RenderPassLoadOp::kDontCare:
        return vk::AttachmentLoadOp::eDontCare;
    default:
        NOTREACHED();
    }
}

vk::AttachmentStoreOp ConvertRenderPassStoreOp(RenderPassStoreOp op)
{
    switch (op) {
    case RenderPassStoreOp::kStore:
        return vk::AttachmentStoreOp::eStore;
    case RenderPassStoreOp::kDontCare:
        return vk::AttachmentStoreOp::eDontCare;
    default:
        NOTREACHED();
    }
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
    rasterizer.lineWidth = 1.0;
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
            default:
                NOTREACHED();
            }
        };

        auto convert_op = [](BlendOp type) {
            switch (type) {
            case BlendOp::kAdd:
                return vk::BlendOp::eAdd;
            default:
                NOTREACHED();
            }
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
    pipeline_info.pDynamicState = &pipelineDynamicStateCreateInfo;

    std::vector<vk::Format> color_formats;
    vk::PipelineRenderingCreateInfo pipeline_rendering_info = {};
    if (m_device.IsDynamicRenderingSupported()) {
        color_formats.resize(render_pass_desc.colors.size());
        for (size_t i = 0; i < color_formats.size(); ++i) {
            color_formats[i] = static_cast<vk::Format>(render_pass_desc.colors[i].format);
        }
        pipeline_rendering_info.colorAttachmentCount = color_formats.size();
        pipeline_rendering_info.pColorAttachmentFormats = color_formats.data();
        if (render_pass_desc.depth_stencil.format != gli::format::FORMAT_UNDEFINED &&
            gli::is_depth(render_pass_desc.depth_stencil.format)) {
            pipeline_rendering_info.depthAttachmentFormat =
                static_cast<vk::Format>(render_pass_desc.depth_stencil.format);
        }
        if (render_pass_desc.depth_stencil.format != gli::format::FORMAT_UNDEFINED &&
            gli::is_stencil(render_pass_desc.depth_stencil.format)) {
            pipeline_rendering_info.stencilAttachmentFormat =
                static_cast<vk::Format>(render_pass_desc.depth_stencil.format);
        }
        pipeline_info.pNext = &pipeline_rendering_info;
    } else {
        m_render_pass = CreateRenderPass(m_device, render_pass_desc);
        pipeline_info.renderPass = m_render_pass.get();
    }

    m_pipeline = m_device.GetDevice().createGraphicsPipelineUnique({}, pipeline_info).value;
}

PipelineType VKGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

vk::RenderPass VKGraphicsPipeline::GetRenderPass() const
{
    return m_render_pass.get();
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
