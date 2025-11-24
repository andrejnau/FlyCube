#include "Pipeline/VKGraphicsPipeline.h"

#include "Device/VKDevice.h"
#include "Utilities/Check.h"
#include "Utilities/NotReached.h"

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

} // namespace

VKGraphicsPipeline::VKGraphicsPipeline(VKDevice& device, const GraphicsPipelineDesc& desc)
    : VKPipeline(device, desc.shaders, desc.layout)
    , desc_(desc)
{
    for (const auto& shader : desc.shaders) {
        if (shader->GetType() == ShaderType::kVertex) {
            CreateInputLayout(shader);
        }
    }

    vk::PipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.vertexBindingDescriptionCount = binding_desc_.size();
    vertex_input_info.pVertexBindingDescriptions = binding_desc_.data();
    vertex_input_info.vertexAttributeDescriptionCount = attribute_desc_.size();
    vertex_input_info.pVertexAttributeDescriptions = attribute_desc_.data();

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
    rasterizer.depthBiasEnable = desc_.rasterizer_desc.depth_bias != 0;
    rasterizer.depthBiasConstantFactor = desc_.rasterizer_desc.depth_bias;
    switch (desc_.rasterizer_desc.fill_mode) {
    case FillMode::kWireframe:
        rasterizer.polygonMode = vk::PolygonMode::eLine;
        break;
    case FillMode::kSolid:
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        break;
    }
    switch (desc_.rasterizer_desc.cull_mode) {
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
    color_blend_attachment.blendEnable = desc_.blend_desc.blend_enable;

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

        color_blend_attachment.srcColorBlendFactor = convert(desc_.blend_desc.blend_src);
        color_blend_attachment.dstColorBlendFactor = convert(desc_.blend_desc.blend_dest);
        color_blend_attachment.colorBlendOp = convert_op(desc_.blend_desc.blend_op);
        color_blend_attachment.srcAlphaBlendFactor = convert(desc_.blend_desc.blend_src_alpha);
        color_blend_attachment.dstAlphaBlendFactor = convert(desc_.blend_desc.blend_dest_apha);
        color_blend_attachment.alphaBlendOp = convert_op(desc_.blend_desc.blend_op_alpha);
    }

    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments(desc_.color_formats.size(),
                                                                               color_blend_attachment);

    vk::PipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = vk::LogicOp::eAnd;
    color_blending.attachmentCount = color_blend_attachments.size();
    color_blending.pAttachments = color_blend_attachments.data();

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.rasterizationSamples = static_cast<vk::SampleCountFlagBits>(desc_.sample_count);
    multisampling.sampleShadingEnable = multisampling.rasterizationSamples != vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.depthTestEnable = desc_.depth_stencil_desc.depth_test_enable;
    depth_stencil.depthWriteEnable = desc_.depth_stencil_desc.depth_write_enable;
    depth_stencil.depthCompareOp = Convert(desc_.depth_stencil_desc.depth_func);
    depth_stencil.depthBoundsTestEnable = desc_.depth_stencil_desc.depth_bounds_test_enable;
    depth_stencil.stencilTestEnable = desc_.depth_stencil_desc.stencil_enable;
    depth_stencil.back = Convert(desc_.depth_stencil_desc.back_face, desc_.depth_stencil_desc.stencil_read_mask,
                                 desc_.depth_stencil_desc.stencil_write_mask);
    depth_stencil.front = Convert(desc_.depth_stencil_desc.front_face, desc_.depth_stencil_desc.stencil_read_mask,
                                  desc_.depth_stencil_desc.stencil_write_mask);

    std::vector<vk::DynamicState> dynamic_state_enables = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    if (device_.IsVariableRateShadingSupported()) {
        dynamic_state_enables.emplace_back(vk::DynamicState::eFragmentShadingRateKHR);
    }

    vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_info = {};
    pipeline_dynamic_state_info.pDynamicStates = dynamic_state_enables.data();
    pipeline_dynamic_state_info.dynamicStateCount = dynamic_state_enables.size();

    vk::GraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.stageCount = shader_stage_create_info_.size();
    pipeline_info.pStages = shader_stage_create_info_.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.pDynamicState = &pipeline_dynamic_state_info;

    if (device_.GetShadingRateImageTileSize() > 0) {
        pipeline_info.flags |= vk::PipelineCreateFlagBits::eRenderingFragmentShadingRateAttachmentKHR;
    }

    vk::PipelineRenderingCreateInfo pipeline_rendering_info = {};
    std::vector<vk::Format> color_formats(desc_.color_formats.size());
    for (size_t i = 0; i < color_formats.size(); ++i) {
        color_formats[i] = static_cast<vk::Format>(desc_.color_formats[i]);
    }
    pipeline_rendering_info.colorAttachmentCount = color_formats.size();
    pipeline_rendering_info.pColorAttachmentFormats = color_formats.data();
    if (desc_.depth_stencil_format != gli::format::FORMAT_UNDEFINED && gli::is_depth(desc_.depth_stencil_format)) {
        pipeline_rendering_info.depthAttachmentFormat = static_cast<vk::Format>(desc_.depth_stencil_format);
    }
    if (desc_.depth_stencil_format != gli::format::FORMAT_UNDEFINED && gli::is_stencil(desc_.depth_stencil_format)) {
        pipeline_rendering_info.stencilAttachmentFormat = static_cast<vk::Format>(desc_.depth_stencil_format);
    }
    pipeline_info.pNext = &pipeline_rendering_info;

    pipeline_ = device_.GetDevice().createGraphicsPipelineUnique({}, pipeline_info).value;
}

PipelineType VKGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

void VKGraphicsPipeline::CreateInputLayout(const std::shared_ptr<Shader>& shader)
{
    std::map<size_t, uint32_t> input_layout_stride;
    for (const auto& vertex : desc_.input) {
        if (!input_layout_stride.contains(vertex.slot)) {
            vk::VertexInputBindingDescription& binding = binding_desc_.emplace_back();
            binding.binding = vertex.slot;
            binding.stride = vertex.stride;
            binding.inputRate = vk::VertexInputRate::eVertex;
            input_layout_stride[vertex.slot] = vertex.stride;
        } else {
            CHECK(input_layout_stride[vertex.slot] == vertex.stride);
        }

        vk::VertexInputAttributeDescription& attribute = attribute_desc_.emplace_back();
        attribute.location = shader->GetInputLayoutLocation(vertex.semantic_name);
        attribute.binding = vertex.slot;
        attribute.format = static_cast<vk::Format>(vertex.format);
        attribute.offset = vertex.offset;
    }
}

const GraphicsPipelineDesc& VKGraphicsPipeline::GetDesc() const
{
    return desc_;
}
