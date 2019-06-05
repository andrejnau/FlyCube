#include "VKProgramApi.h"

#include <vector>
#include <list>
#include <utility>
#include <Resource/VKResource.h>
#include <View/VKView.h>
#include <Shader/SpirvCompiler.h>
#include <iostream>
#include <Utilities/VKUtility.h>

VKProgramApi::VKProgramApi(VKContext& context)
    : CommonProgramApi(context)
    , m_context(context)
    , m_view_creater(context, *this)
{
    m_depth_stencil_desc.depth_enable = true;
}

VkShaderStageFlagBits VKProgramApi::ShaderType2Bit(ShaderType type)
{
    switch (type)
    {
    case ShaderType::kVertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderType::kPixel:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderType::kGeometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderType::kCompute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return {};
}

void VKProgramApi::LinkProgram()
{
    ParseShaders();
    m_view_creater.OnLinkProgram();

    for (auto & shader : m_shaders_info)
    {
        shaderStageCreateInfo.emplace_back();
        shaderStageCreateInfo.back().stage = ShaderType2Bit(shader.first);

        shaderStageCreateInfo.back().sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.back().module = m_shaders[shader.first];
        shaderStageCreateInfo.back().pName = shader.second.c_str();
        shaderStageCreateInfo.back().pSpecializationInfo = NULL;
    }

    if (m_spirv.count(ShaderType::kVertex))
    {
        CreateInputLayout(m_spirv[ShaderType::kVertex], binding_desc, attribute_desc);
    }
    if (m_spirv.count(ShaderType::kPixel))
    {
        CreateRenderPass(m_spirv[ShaderType::kPixel]);
    }
}

void VKProgramApi::CreateGrPipeLine()
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = binding_desc.size();
    vertexInputInfo.pVertexBindingDescriptions = binding_desc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = attribute_desc.size();
    vertexInputInfo.pVertexAttributeDescriptions = attribute_desc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = m_rasterizer_desc.DepthBias != 0;
    rasterizer.depthBiasConstantFactor = m_rasterizer_desc.DepthBias;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = m_blend_desc.blend_enable;

    if (colorBlendAttachment.blendEnable)
    {
        auto convert = [](Blend type)
        {
            switch (type)
            {
            case Blend::kZero:
                return VK_BLEND_FACTOR_ZERO;
            case Blend::kSrcAlpha:
                return VK_BLEND_FACTOR_SRC_ALPHA;
            case Blend::kInvSrcAlpha:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            }
            throw std::runtime_error("unsupported");
        };

        auto convert_op = [](BlendOp type)
        {
            switch (type)
            {
            case BlendOp::kAdd:
                return VK_BLEND_OP_ADD;
            }
            throw std::runtime_error("unsupported");
        };

        colorBlendAttachment.srcColorBlendFactor = convert(m_blend_desc.blend_src);
        colorBlendAttachment.dstColorBlendFactor = convert(m_blend_desc.blend_dest);
        colorBlendAttachment.colorBlendOp = convert_op(m_blend_desc.blend_op);
        colorBlendAttachment.srcAlphaBlendFactor = convert(m_blend_desc.blend_src_alpha);
        colorBlendAttachment.dstAlphaBlendFactor = convert(m_blend_desc.blend_dest_apha);
        colorBlendAttachment.alphaBlendOp = convert_op(m_blend_desc.blend_op_alpha);
    }

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_attachment_views.size() - 1, colorBlendAttachment);

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_AND;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = (VkSampleCountFlagBits)msaa_count;
    multisampling.sampleShadingEnable = multisampling.rasterizationSamples != VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_depth_stencil_desc.depth_enable;
    depthStencil.depthWriteEnable = m_depth_stencil_desc.depth_enable;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStageCreateInfo.size();
    pipelineInfo.pStages = shaderStageCreateInfo.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;

    pipelineInfo.layout = m_pipeline_layout;

    pipelineInfo.renderPass = m_render_pass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
    pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();

    pipelineInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

    if (vkCreateGraphicsPipelines(m_context.m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to vkCreateGraphicsPipelines");
    }
}

void VKProgramApi::CreateComputePipeLine()
{
    VkComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = shaderStageCreateInfo.front();
    pipeline_info.layout = m_pipeline_layout;
    ASSERT_SUCCEEDED(vkCreateComputePipelines(m_context.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphicsPipeline));
}

void VKProgramApi::CreatePipeLine()
{
    if (m_is_compute)
        CreateComputePipeLine();
    else
        CreateGrPipeLine();
}

void VKProgramApi::UseProgram()
{
}

void VKProgramApi::ApplyBindings()
{
    UpdateCBuffers();

    if (m_changed_om)
    {
        if (!m_is_compute)
        {
            for (int i = 0; i < m_attachment_descriptions.size() - 1; ++i)
            {
                m_attachment_descriptions[i].loadOp = m_clear_cache.GetColorLoadOp(i);
                m_clear_cache.GetColorLoadOp(i) = VK_ATTACHMENT_LOAD_OP_LOAD;
            }
            if (m_attachment_views.back())
                m_attachment_descriptions.back().loadOp = m_clear_cache.GetDepthLoadOp();
            m_clear_cache.GetDepthLoadOp() = VK_ATTACHMENT_LOAD_OP_LOAD;
            VkSubpassDescription subPass = {};
            subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subPass.colorAttachmentCount = m_attachment_references.size() - 1;
            subPass.pColorAttachments = m_attachment_references.data();
            subPass.pDepthStencilAttachment = &m_attachment_references.back();
            if (!m_attachment_views.back())
                subPass.pDepthStencilAttachment = nullptr;

            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = m_attachment_descriptions.size();
            if (!m_attachment_views.back())
                --renderPassInfo.attachmentCount;
            renderPassInfo.pAttachments = m_attachment_descriptions.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subPass;

           VkAccessFlags AccessMask =
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_SHADER_WRITE_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_TRANSFER_READ_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT |
                VK_ACCESS_HOST_READ_BIT |
                VK_ACCESS_HOST_WRITE_BIT |
                VK_ACCESS_MEMORY_READ_BIT |
                VK_ACCESS_MEMORY_WRITE_BIT;

            VkSubpassDependency self_dependencie = {};
            self_dependencie.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            self_dependencie.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            self_dependencie.srcAccessMask = AccessMask;
            self_dependencie.dstAccessMask = AccessMask;
            self_dependencie.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &self_dependencie;

            vkCreateRenderPass(m_context.m_device, &renderPassInfo, nullptr, &m_render_pass);

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_render_pass;
            framebufferInfo.attachmentCount = m_attachment_views.size();
            if (!m_attachment_views.back())
                --framebufferInfo.attachmentCount;
            framebufferInfo.pAttachments = m_attachment_views.data();
            framebufferInfo.width = m_rtv_size[0].first.width;
            framebufferInfo.height = m_rtv_size[0].first.height;
            framebufferInfo.layers = m_rtv_size[0].second;

            if (vkCreateFramebuffer(m_context.m_device, &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        m_changed_om = false;
        CreatePipeLine();
    }

    if (m_is_compute)
        vkCmdBindPipeline(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_COMPUTE, graphicsPipeline);
    else
        vkCmdBindPipeline(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    if (!m_descriptor_sets.empty())
    {
        m_descriptor_sets.clear();
    }

    for (size_t i = 0; i < m_descriptor_set_layouts.size(); ++i)
    {
        m_descriptor_sets.emplace_back(m_context.GetDescriptorPool().AllocateDescriptorSet(m_descriptor_set_layouts[i], m_descriptor_count_by_set[i]));
    }
   
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::list<VkDescriptorImageInfo> list_image_info;
    std::list<VkDescriptorBufferInfo> list_buffer_info;
    
    for (auto& x : m_bound_resources)
    {
        bool is_rtv_dsv = false;
        switch (x.first.res_type)
        {
        case ResourceType::kRtv:
        case ResourceType::kDsv:
            is_rtv_dsv = true;
            break;
        }

        if (is_rtv_dsv || !x.second.res)
            continue;

        auto& view = static_cast<VKView&>(*x.second.view);
        ShaderType shader_type = x.first.shader_type;
        ShaderRef& shader_ref = m_shader_ref.find(shader_type)->second;
        std::string name = GetBindingName(x.first);
        if (name == "$Globals")
            name = "_Global";

        if (!shader_ref.resources.count(name))
            throw std::runtime_error("failed to find resource reflection");
        auto ref_res = shader_ref.resources[name];
        VKResource& res = static_cast<VKResource&>(*x.second.res);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptor_sets[GetSetNumByShaderType(shader_type)];
        descriptorWrite.dstBinding = ref_res.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = ref_res.descriptor_type;
        descriptorWrite.descriptorCount = 1;

        switch (ref_res.descriptor_type)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            list_image_info.emplace_back();
            VkDescriptorImageInfo& image_info = list_image_info.back();
            image_info.sampler = res.sampler.res;
            descriptorWrite.pImageInfo = &image_info;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            list_image_info.emplace_back();
            VkDescriptorImageInfo& image_info = list_image_info.back();
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = view.srv;
            descriptorWrite.pImageInfo = &image_info;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            list_image_info.emplace_back();
            VkDescriptorImageInfo& image_info = list_image_info.back();
            image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            image_info.imageView = view.srv;
            descriptorWrite.pImageInfo = &image_info;
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            list_buffer_info.emplace_back();
            VkDescriptorBufferInfo& buffer_info = list_buffer_info.back();
            buffer_info.buffer = res.buffer.res;
            buffer_info.offset = 0;
            buffer_info.range = VK_WHOLE_SIZE;
            descriptorWrite.pBufferInfo = &buffer_info;
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        default:
            ASSERT(false);
            break;
        }

        if (descriptorWrite.pImageInfo || descriptorWrite.pBufferInfo)
            descriptorWrites.push_back(descriptorWrite);
    }

    if (!descriptorWrites.empty())
        vkUpdateDescriptorSets(m_context.m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    if (m_is_compute)
        vkCmdBindDescriptorSets(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0,
            m_descriptor_sets.size(), m_descriptor_sets.data(), 0, nullptr);
    else
        vkCmdBindDescriptorSets(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0,
            m_descriptor_sets.size(), m_descriptor_sets.data(), 0, nullptr);
}

View::Ptr VKProgramApi::CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res)
{
    return m_view_creater.GetView(m_program_id, bind_key.shader_type, bind_key.res_type, bind_key.slot, view_desc, GetBindingName(bind_key), res);
}

void VKProgramApi::RenderPassBegin()
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_render_pass;
    renderPassInfo.framebuffer = m_framebuffer;

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_rtv_size[0].first;

    std::vector<VkClearValue> clearValues(m_attachment_views.size() - 1);
    for (int i = 0; i < m_attachment_views.size() - 1; ++i)
    {
        clearValues[i].color = m_clear_cache.GetColor(i);
    }
    if (m_attachment_views.back())
    {
        clearValues.emplace_back();
        clearValues.back().depthStencil = m_clear_cache.GetDepth();
    }

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_context.m_cmd_bufs[m_context.GetFrameIndex()], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VKProgramApi::CompileShader(const ShaderBase& shader)
{
    if (shader.type == ShaderType::kCompute)
        m_is_compute = true;
    SpirvOption option;
    option.auto_map_bindings = true;
    option.hlsl_iomap = true;
    option.invert_y = true;
    if (m_shader_types.count(ShaderType::kGeometry) && shader.type == ShaderType::kVertex)
        option.invert_y = false;
    option.resource_set_binding = GetSetNumByShaderType(shader.type);
    auto spirv = SpirvCompile(shader, option);
    m_spirv[shader.type] = spirv;

    VkShaderModuleCreateInfo vertexShaderCreationInfo = {};
    vertexShaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderCreationInfo.codeSize = sizeof(uint32_t) * spirv.size();
    vertexShaderCreationInfo.pCode = spirv.data();

    VkShaderModule shaderModule;
    auto result = vkCreateShaderModule(m_context.m_device, &vertexShaderCreationInfo, nullptr, &shaderModule);
    m_shaders[shader.type] = shaderModule;
    m_shaders_info[shader.type] = shader.entrypoint;
    m_shaders_info2[shader.type] = &shader;
}

static void print_resources(const spirv_cross::Compiler &compiler, const char *tag, const spirv_cross::SmallVector<spirv_cross::Resource> &resources)
{
    using namespace spirv_cross;
    using namespace spv;
    using namespace std;
    fprintf(stderr, "%s\n", tag);
    fprintf(stderr, "=============\n\n");
    bool print_ssbo = !strcmp(tag, "ssbos");

    for (auto &res : resources)
    {
        auto &type = compiler.get_type(res.type_id);

        if (print_ssbo && compiler.buffer_is_hlsl_counter_buffer(res.id))
            continue;

        // If we don't have a name, use the fallback for the type instead of the variable
        // for SSBOs and UBOs since those are the only meaningful names to use externally.
        // Push constant blocks are still accessed by name and not block name, even though they are technically Blocks.
        bool is_push_constant = compiler.get_storage_class(res.id) == StorageClassPushConstant;
        bool is_block = compiler.get_decoration_bitset(type.self).get(DecorationBlock) ||
            compiler.get_decoration_bitset(type.self).get(DecorationBufferBlock);
        bool is_sized_block = is_block && (compiler.get_storage_class(res.id) == StorageClassUniform ||
            compiler.get_storage_class(res.id) == StorageClassUniformConstant);
        uint32_t fallback_id = !is_push_constant && is_block ? res.base_type_id : res.id;

        uint32_t block_size = 0;
        if (is_sized_block)
            block_size = uint32_t(compiler.get_declared_struct_size(compiler.get_type(res.base_type_id)));

        Bitset mask;
        if (print_ssbo)
            mask = compiler.get_buffer_block_flags(res.id);
        else
            mask = compiler.get_decoration_bitset(res.id);

        string array;
        for (auto arr : type.array)
            array = join("[", arr ? convert_to_string(arr) : "", "]") + array;

        fprintf(stderr, " ID %03u : %s%s", res.id,
            !res.name.empty() ? res.name.c_str() : compiler.get_fallback_name(fallback_id).c_str(), array.c_str());

        if (mask.get(DecorationLocation))
            fprintf(stderr, " (Location : %u)", compiler.get_decoration(res.id, DecorationLocation));
        if (mask.get(DecorationDescriptorSet))
            fprintf(stderr, " (Set : %u)", compiler.get_decoration(res.id, DecorationDescriptorSet));
        if (mask.get(DecorationBinding))
            fprintf(stderr, " (Binding : %u)", compiler.get_decoration(res.id, DecorationBinding));
        if (mask.get(DecorationInputAttachmentIndex))
            fprintf(stderr, " (Attachment : %u)", compiler.get_decoration(res.id, DecorationInputAttachmentIndex));
        if (mask.get(DecorationNonReadable))
            fprintf(stderr, " writeonly");
        if (mask.get(DecorationNonWritable))
            fprintf(stderr, " readonly");
        if (is_sized_block)
            fprintf(stderr, " (BlockSize : %u bytes)", block_size);

        uint32_t counter_id = 0;
        if (print_ssbo && compiler.buffer_get_hlsl_counter_buffer(res.id, counter_id))
            fprintf(stderr, " (HLSL counter buffer ID: %u)", counter_id);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "=============\n\n");
}

void VKProgramApi::ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary, std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    m_shader_ref.emplace(shader_type, spirv_binary);
    spirv_cross::CompilerHLSL& compiler = m_shader_ref.find(shader_type)->second.compiler;
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    auto generate_bindings = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources, VkDescriptorType res_type)
    {
        for (auto& res : resources)
        {
            auto& info = m_shader_ref.find(shader_type)->second.resources[res.name];
            info.res = res;
            auto &type = compiler.get_type(res.base_type_id);

            if (type.basetype == spirv_cross::SPIRType::BaseType::Image && type.image.dim == spv::Dim::DimBuffer)
            {
                if (res_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
                {
                    res_type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                }
                else if (res_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                {
                    res_type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                }
            }

            bindings.emplace_back();
            VkDescriptorSetLayoutBinding& binding = bindings.back();
            binding.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            binding.descriptorType = res_type;
            binding.descriptorCount = 1;
            binding.stageFlags = ShaderType2Bit(shader_type);

            info.binding = binding.binding;
            info.descriptor_type = res_type;
        }
    };

    generate_bindings(resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    generate_bindings(resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    generate_bindings(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);
    generate_bindings(resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    generate_bindings(resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    print_resources(compiler, " stage_inputs ", resources.stage_inputs);
    print_resources(compiler, " uniform_buffers ", resources.uniform_buffers);
    print_resources(compiler, " storage_buffers ", resources.storage_buffers);
    print_resources(compiler, " storage_images ", resources.storage_images);
    print_resources(compiler, " separate_images ", resources.separate_images);
    print_resources(compiler, " separate_samplers ", resources.separate_samplers);
    print_resources(compiler, " stage_outputs ", resources.stage_outputs);
}

size_t VKProgramApi::GetSetNumByShaderType(ShaderType type)
{
    if (m_shader_type2set.count(type))
        return m_shader_type2set[type];
    size_t num = m_shader_type2set.size();
    m_shader_type2set[type] = num;
    return num;
}

void VKProgramApi::ParseShaders()
{
    for (auto & spirv_it : m_spirv)
    {
        auto& spirv = spirv_it.second;
        std::cout << std::endl << m_shaders_info2[spirv_it.first]->shader_path << std::endl;

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        ParseShader(spirv_it.first, spirv, bindings);

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        int set_num = GetSetNumByShaderType(spirv_it.first);
        if (m_descriptor_set_layouts.size() <= set_num)
        {
            m_descriptor_set_layouts.resize(set_num + 1);
            m_descriptor_count_by_set.resize(set_num + 1);
        }

        VkDescriptorSetLayout& descriptor_set_layout = m_descriptor_set_layouts[set_num];
        if (vkCreateDescriptorSetLayout(m_context.m_device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        for (auto & binding : bindings)
        {
            m_descriptor_count_by_set[set_num][binding.descriptorType] += binding.descriptorCount;
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = m_descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = m_descriptor_set_layouts.data();

    if (vkCreatePipelineLayout(m_context.m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to vkCreatePipelineLayout");
    }
}

void VKProgramApi::OnPresent()
{
    m_cbv_offset.get().clear();
    m_changed_om = true;
}

ShaderBlob VKProgramApi::GetBlobByType(ShaderType type) const
{
    auto it = m_spirv.find(type);
    if (it == m_spirv.end())
        return {};

    return { (uint8_t*)it->second.data(), sizeof(uint32_t) * it->second.size() };
    return ShaderBlob();
}

void VKProgramApi::OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    if (res.image.res != VK_NULL_HANDLE)
        m_context.TransitionImageLayout(res.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, view_desc);
}

void VKProgramApi::OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr& ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    if (res.image.res != VK_NULL_HANDLE)
        m_context.TransitionImageLayout(res.image, VK_IMAGE_LAYOUT_GENERAL, view_desc);
}

void VKProgramApi::OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & ires)
{
}

void VKProgramApi::OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
}

VKView* ConvertView(const View::Ptr& view)
{
    if (!view)
        return nullptr;
    return &static_cast<VKView&>(*view);
}

void VKProgramApi::OnAttachRTV(uint32_t slot, const ViewDesc& view_desc, const Resource::Ptr & ires)
{
    m_changed_om = true;
    if (m_context.m_is_open_render_pass)
    {
        vkCmdEndRenderPass(m_context.m_cmd_bufs[m_context.GetFrameIndex()]);
        m_context.m_is_open_render_pass = false;
    }

    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    m_context.TransitionImageLayout(res.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, view_desc);

    auto view = ConvertView(FindView(ShaderType::kPixel, ResourceType::kRtv, slot));
    m_attachment_views[slot] = view->om;
    m_rtv_size[slot] = { { res.image.size.width >> view_desc.level, res.image.size.height >> view_desc.level}, res.image.array_layers };
 
    VkAttachmentDescription& colorAttachment = m_attachment_descriptions[slot];
    colorAttachment.format = res.image.format;
    colorAttachment.samples = (VkSampleCountFlagBits)res.image.msaa_count;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    msaa_count = res.image.msaa_count;

    VkAttachmentReference& colorAttachmentRef = m_attachment_references[slot];
    colorAttachmentRef.attachment = slot;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
}

void VKProgramApi::OnAttachDSV(const ViewDesc& view_desc, const Resource::Ptr & ires)
{
    m_changed_om = true;
    if (m_context.m_is_open_render_pass)
    {
        vkCmdEndRenderPass(m_context.m_cmd_bufs[m_context.GetFrameIndex()]);
        m_context.m_is_open_render_pass = false;
    }

    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    m_context.TransitionImageLayout(res.image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, view_desc);

    auto view = ConvertView(FindView(ShaderType::kPixel, ResourceType::kDsv, 0));

    m_attachment_views.back() = view->om;
    m_rtv_size.back() = { { res.image.size.width >> view_desc.level, (res.image.size.height >> view_desc.level)}, res.image.array_layers };

    auto& m_depth_attachment = m_attachment_descriptions.back();
    m_depth_attachment.format = res.image.format;
    m_depth_attachment.samples = (VkSampleCountFlagBits)res.image.msaa_count;
    m_depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    msaa_count = res.image.msaa_count;

    VkAttachmentReference& depthAttachmentReference = m_attachment_references.back();
    depthAttachmentReference.attachment = m_attachment_descriptions.size() - 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

void VKProgramApi::ClearRenderTarget(uint32_t slot, const std::array<float, 4>& color)
{
    auto& clear_color = m_clear_cache.GetColor(slot);
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];
    m_clear_cache.GetColorLoadOp(slot) = VK_ATTACHMENT_LOAD_OP_CLEAR;
}

void VKProgramApi::ClearDepthStencil(uint32_t ClearFlags, float Depth, uint8_t Stencil)
{
    m_clear_cache.GetDepth() = { Depth, Stencil };
    m_clear_cache.GetDepthLoadOp() = VK_ATTACHMENT_LOAD_OP_CLEAR;
}

void VKProgramApi::SetRasterizeState(const RasterizerDesc& desc)
{
    m_rasterizer_desc = desc;
    m_changed_om = true;
}

void VKProgramApi::SetBlendState(const BlendDesc& desc)
{
    m_blend_desc = desc;
    m_changed_om = true;
}

void VKProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
    m_depth_stencil_desc = desc;
    m_changed_om = true;
}

void VKProgramApi::CreateInputLayout(
    const std::vector<uint32_t>& spirv_binary,
    std::vector<VkVertexInputBindingDescription>& binding_desc,
    std::vector<VkVertexInputAttributeDescription>& attribute_desc)
{
    spirv_cross::CompilerHLSL compiler(std::move(spirv_binary));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (auto& resource : resources.stage_inputs)
    {
        auto &type = compiler.get_type(resource.base_type_id);
        unsigned location = compiler.get_decoration(resource.id, spv::DecorationLocation);

        binding_desc.emplace_back();
        auto& binding = binding_desc.back();
        attribute_desc.emplace_back();
        auto& attribute = attribute_desc.back();

        attribute.binding = location;
        attribute.location = location;
        binding.binding = location;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        binding.stride = type.vecsize * type.width / 8;

        if (type.basetype == spirv_cross::SPIRType::Float)
        {
            if (type.vecsize == 1)
                attribute.format = VK_FORMAT_R32_SFLOAT;
            else if (type.vecsize == 2)
                attribute.format = VK_FORMAT_R32G32_SFLOAT;
            else if (type.vecsize == 3)
                attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            else if (type.vecsize == 4)
                attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else if (type.basetype == spirv_cross::SPIRType::UInt)
        {
            if (type.vecsize == 1)
                attribute.format = VK_FORMAT_R32_UINT;
            else if (type.vecsize == 2)
                attribute.format = VK_FORMAT_R32G32_UINT;
            else if (type.vecsize == 3)
                attribute.format = VK_FORMAT_R32G32B32_UINT;
            else if (type.vecsize == 4)
                attribute.format = VK_FORMAT_R32G32B32A32_UINT;
        }
        else if (type.basetype == spirv_cross::SPIRType::Int)
        {
            if (type.vecsize == 1)
                attribute.format = VK_FORMAT_R32_SINT;
            else if (type.vecsize == 2)
                attribute.format = VK_FORMAT_R32G32_SINT;
            else if (type.vecsize == 3)
                attribute.format = VK_FORMAT_R32G32B32_SINT;
            else if (type.vecsize == 4)
                attribute.format = VK_FORMAT_R32G32B32A32_SINT;
        }
    }
}

void VKProgramApi::CreateRenderPass(const std::vector<uint32_t>& spirv_binary)
{
    spirv_cross::CompilerHLSL compiler(std::move(spirv_binary));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (auto& resource : resources.stage_outputs)
    {
        auto &type = compiler.get_type(resource.base_type_id);
        size_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        m_num_rtv = std::max(m_num_rtv, location + 1);
    }

    m_attachment_descriptions.resize(m_num_rtv + 1);
    m_attachment_references.resize(m_num_rtv + 1);
    m_attachment_views.resize(m_num_rtv + 1);
    m_rtv_size.resize(m_num_rtv + 1);
}
