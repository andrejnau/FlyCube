#include "Pipeline/VKGraphicsPipeline.h"
#include <Device/VKDevice.h>
#include <Program/VKProgram.h>
#include <Shader/SpirvShader.h>
#include <map>

vk::ShaderStageFlagBits ExecutionModel2Bit(spv::ExecutionModel model)
{
    switch (model)
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case spv::ExecutionModel::ExecutionModelFragment:
        return vk::ShaderStageFlagBits::eFragment;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case spv::ExecutionModel::ExecutionModelGLCompute:
        return vk::ShaderStageFlagBits::eCompute;
    case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        return vk::ShaderStageFlagBits::eRaygenNV;
    case spv::ExecutionModel::ExecutionModelIntersectionNV:
        return vk::ShaderStageFlagBits::eIntersectionNV;
    case spv::ExecutionModel::ExecutionModelAnyHitNV:
        return vk::ShaderStageFlagBits::eAnyHitNV;
    case spv::ExecutionModel::ExecutionModelClosestHitNV:
        return vk::ShaderStageFlagBits::eClosestHitNV;
    case spv::ExecutionModel::ExecutionModelMissNV:
        return vk::ShaderStageFlagBits::eMissNV;
    case spv::ExecutionModel::ExecutionModelCallableNV:
        return vk::ShaderStageFlagBits::eCallableNV;
    case spv::ExecutionModel::ExecutionModelTaskNV:
        return vk::ShaderStageFlagBits::eTaskNV;
    case spv::ExecutionModel::ExecutionModelMeshNV:
        return vk::ShaderStageFlagBits::eMeshNV;
    }
    assert(false);
    return {};
}

VKGraphicsPipeline::VKGraphicsPipeline(VKDevice& device, const GraphicsPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) vk_program = desc.program->As<VKProgram>();
    auto shaders = vk_program.GetShaders();
    for (auto& shader : shaders)
    {
        ShaderType shader_type = shader->GetType();
        auto blob = shader->GetBlob();
        switch (shader_type)
        {
        case ShaderType::kVertex:
            CreateInputLayout(blob, m_binding_desc, m_attribute_desc);
            break;
        }

        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = sizeof(uint32_t) * blob.size();
        shader_module_info.pCode = blob.data();

        m_shader_modules[shader_type] = m_device.GetDevice().createShaderModuleUnique(shader_module_info);

        spirv_cross::CompilerHLSL compiler(blob);
        m_entries[shader_type] = compiler.get_entry_points_and_stages();

        for (auto& entry_point : m_entries[shader_type])
        {
            m_shader_stage_create_info.emplace_back();
            m_shader_stage_create_info.back().stage = ExecutionModel2Bit(entry_point.execution_model);
            m_shader_stage_create_info.back().module = m_shader_modules[shader_type].get();
            m_shader_stage_create_info.back().pName = entry_point.name.c_str();
            m_shader_stage_create_info.back().pSpecializationInfo = NULL;
        }
    }

    CreateGrPipeLine();
}

PipelineType VKGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

vk::Pipeline VKGraphicsPipeline::GetPipeline() const
{
    return m_pipeline.get();
}

vk::RenderPass VKGraphicsPipeline::GetRenderPass() const
{
    return m_desc.render_pass->As<VKRenderPass>().GetRenderPass();
}

void VKGraphicsPipeline::CreateInputLayout(const std::vector<uint32_t>& spirv_binary,
                                   std::vector<vk::VertexInputBindingDescription>& m_binding_desc,
                                   std::vector<vk::VertexInputAttributeDescription>& m_attribute_desc)
{
    spirv_cross::CompilerHLSL compiler(spirv_binary);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    std::map<std::string, uint32_t> locations;
    for (auto& resource : resources.stage_inputs)
    {
        std::string semantic = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
        locations[semantic] = compiler.get_decoration(resource.id, spv::DecorationLocation);
    }

    for (auto& vertex : m_desc.input)
    {
        m_binding_desc.emplace_back();
        auto& binding = m_binding_desc.back();
        m_attribute_desc.emplace_back();
        auto& attribute = m_attribute_desc.back();
        attribute.location = locations.at(vertex.semantic_name);
        attribute.binding = binding.binding = vertex.slot;
        binding.inputRate = vk::VertexInputRate::eVertex;
        binding.stride = vertex.stride;
        attribute.format = static_cast<vk::Format>(vertex.format);
    }
}

void VKGraphicsPipeline::CreateGrPipeLine()
{
    const RenderPassDesc& render_pass_desc = m_desc.render_pass->GetDesc();
    decltype(auto) vk_program = m_desc.program->As<VKProgram>();

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
    switch (m_desc.rasterizer_desc.fill_mode)
    {
    case FillMode::kWireframe:
        rasterizer.polygonMode = vk::PolygonMode::eLine;
        break;
    case FillMode::kSolid:
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        break;
    }
    switch (m_desc.rasterizer_desc.cull_mode)
    {
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
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = m_desc.blend_desc.blend_enable;

    if (color_blend_attachment.blendEnable)
    {
        auto convert = [](Blend type)
        {
            switch (type)
            {
            case Blend::kZero:
                return vk::BlendFactor::eZero;
            case Blend::kSrcAlpha:
                return vk::BlendFactor::eSrcAlpha;
            case Blend::kInvSrcAlpha:
                return vk::BlendFactor::eOneMinusSrcAlpha;
            }
            throw std::runtime_error("unsupported");
        };

        auto convert_op = [](BlendOp type)
        {
            switch (type)
            {
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

    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments(render_pass_desc.colors.size(), color_blend_attachment);

    vk::PipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = vk::LogicOp::eAnd;
    color_blending.attachmentCount = color_blend_attachments.size();
    color_blending.pAttachments = color_blend_attachments.data();

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.rasterizationSamples = static_cast<vk::SampleCountFlagBits>(render_pass_desc.sample_count);
    multisampling.sampleShadingEnable = multisampling.rasterizationSamples != vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.depthTestEnable = m_desc.depth_desc.depth_enable;
    depth_stencil.depthWriteEnable = m_desc.depth_desc.depth_enable;
    if (m_desc.depth_desc.func == DepthComparison::kLess)
        depth_stencil.depthCompareOp = vk::CompareOp::eLess;
    else if (m_desc.depth_desc.func == DepthComparison::kLessEqual)
        depth_stencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    std::vector<vk::DynamicState> dynamic_state_enables = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
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
    pipeline_info.layout = vk_program.GetPipelineLayout();
    pipeline_info.renderPass = GetRenderPass();
    pipeline_info.pDynamicState = &pipelineDynamicStateCreateInfo;
    m_pipeline = m_device.GetDevice().createGraphicsPipelineUnique({}, pipeline_info);
}
