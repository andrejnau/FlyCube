#include "VKProgramApi.h"

#include <vector>
#include <utility>
#include <Context/VKResource.h>

VKProgramApi::VKProgramApi(VKContext& context)
    : m_context(context)
    , m_descriptor_sets(context)
{
}

void VKProgramApi::SetMaxEvents(size_t)
{
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
    if (m_shaders_info.count(ShaderType::kCompute))
        return;

    ParseShaders();
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
        auto& spirv = m_spirv[ShaderType::kVertex];
        assert(spirv.size() % 4 == 0);
        std::vector<uint32_t> spirv32((uint32_t*)spirv.data(), ((uint32_t*)spirv.data()) + spirv.size() / 4);
        CreateInputLayout(spirv32, binding_desc, attribute_desc);
    }
    if (m_spirv.count(ShaderType::kPixel))
    {
        auto& spirv = m_spirv[ShaderType::kPixel];
        assert(spirv.size() % 4 == 0);
        std::vector<uint32_t> spirv32((uint32_t*)spirv.data(), ((uint32_t*)spirv.data()) + spirv.size() / 4);
        CreateRenderPass(spirv32);
    }
}

void VKProgramApi::CreateGrPipiLine()
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

    VkExtent2D swapChainExtent = { 1280, 720 };

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

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_rtv.size() - 1, colorBlendAttachment);

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
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

    pipelineInfo.renderPass = renderPass;
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

void VKProgramApi::UseProgram()
{
    m_context.UseProgram(*this);
}

void VKProgramApi::ApplyBindings()
{
    for (auto &x : m_cbv_layout)
    {
        BufferLayout& buffer = x.second;
        auto& res = m_cbv_buffer[x.first];
        if (buffer.SyncData())
        {
            m_context.UpdateSubresource(res, 0, buffer.GetBuffer().data(), 0, 0);
        }

        AttachCBV(std::get<0>(x.first), std::get<2>(x.first), std::get<1>(x.first), res);
    }

    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.colorAttachmentCount = m_color_attachments_ref.size() - 1;
    subPass.pColorAttachments = m_color_attachments_ref.data();
    subPass.pDepthStencilAttachment = &m_color_attachments_ref.back();

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = m_color_attachments.size();
    renderPassInfo.pAttachments = m_color_attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;

    vkCreateRenderPass(m_context.m_device, &renderPassInfo, nullptr, &renderPass);

    CreateGrPipiLine();

    vkCmdBindPipeline(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdBindDescriptorSets(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 
        m_descriptor_sets.get().size(), m_descriptor_sets.get().data(), 0, nullptr);

    {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = m_rtv.size();
        framebufferInfo.pAttachments = m_rtv.data();
        framebufferInfo.width = m_rtv_size[0].width;
        framebufferInfo.height = m_rtv_size[0].height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_context.m_device, &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VKProgramApi::RenderPassBegin()
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = m_framebuffer;

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_rtv_size[0];

    VkClearColorValue clearColor = { 0.0f, 0.2f, 0.4f, 1.0f };
    std::vector<VkClearValue> clearValues(m_rtv.size() - 1);
    for (int i = 0; i < clearValues.size(); ++i)
    {
        clearValues[i].color = clearColor;
    }
    clearValues.emplace_back();
    clearValues.back().depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_context.m_cmd_bufs[m_context.GetFrameIndex()], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VKProgramApi::RenderPassEnd()
{
    vkCmdEndRenderPass(m_context.m_cmd_bufs[m_context.GetFrameIndex()]);
}


std::vector<uint8_t> readFile(const char* filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<unsigned char> fileData(fileSize);
    file.read((char*)&fileData[0], fileSize);
    return fileData;
}

std::string get_tmp_file(const std::string& prefix)
{
    char tmpdir[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tmpdir);
    return tmpdir + prefix;
}

std::vector<uint8_t> VKProgramApi::hlsl2spirv(const ShaderBase& shader)
{
    std::string shader_type;
    switch (shader.type)
    {
    case ShaderType::kPixel:
        shader_type = "frag";
        break;
    case ShaderType::kVertex:
        shader_type = "vert";
        break;
    case ShaderType::kGeometry:
        shader_type = "geom";
        break;
    case ShaderType::kCompute:
        shader_type = "comp";
        break;
    }

    std::string glsl_name = shader.shader_path;
    glsl_name = glsl_name.substr(glsl_name.find_last_of("\\/")+1);
    glsl_name = glsl_name.replace(glsl_name.find(".hlsl"), 5, ".glsl");
    std::string spirv_path = get_tmp_file("SponzaApp.spirv");

    std::string cmd = "C:\\VulkanSDK\\1.1.73.0\\Bin\\glslangValidator.exe";
    cmd += " --auto-map-bindings --hlsl-iomap ";
    cmd += " --resource-set-binding " + std::to_string(GetSetNumByShaderType(shader.type)) + " ";
    cmd += " -g ";
    cmd += " -e ";
    cmd += shader.entrypoint;
    cmd += " -S ";
    cmd += shader_type;
    cmd += " -V -D ";
    cmd += GetAssetFullPath(shader.shader_path);
    cmd += " -o ";
    cmd += spirv_path;

    for (auto &x : shader.define)
    {
        cmd += " -D" + x.first + "=" + x.second;
    }

    DeleteFileA(spirv_path.c_str());
    system(cmd.c_str());

    auto res = readFile(spirv_path.c_str());

    DeleteFileA(spirv_path.c_str());
    return res;
}

void VKProgramApi::CompileShader(const ShaderBase& shader)
{
    auto spirv = hlsl2spirv(shader);
    m_spirv[shader.type] = spirv;

    VkShaderModuleCreateInfo vertexShaderCreationInfo = {};
    vertexShaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderCreationInfo.codeSize = spirv.size();
    vertexShaderCreationInfo.pCode = (uint32_t *)spirv.data();

    VkShaderModule shaderModule;
    auto result = vkCreateShaderModule(m_context.m_device, &vertexShaderCreationInfo, nullptr, &shaderModule);
    if (result)
    {
        int b = 0;
    }
    m_shaders[shader.type] = shaderModule;
    m_shaders_info[shader.type] = shader.entrypoint;
    m_shaders_info2[shader.type] = &shader;

 

}

#include <iostream>

static void print_resources(const spirv_cross::Compiler &compiler, const char *tag, const std::vector<spirv_cross::Resource> &resources)
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

    auto generate_bindings = [&](const std::vector<spirv_cross::Resource>& resources, VkDescriptorType res_type)
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
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

    std::map<VkDescriptorType, size_t> descriptor_count;
    for (auto & spirv_it : m_spirv)
    {
        auto& spirv = spirv_it.second;
        assert(spirv.size() % 4 == 0);
        std::vector<uint32_t> spirv32((uint32_t*)spirv.data(), ((uint32_t*)spirv.data()) + spirv.size() / 4);
        std::cout << std::endl << m_shaders_info2[spirv_it.first]->shader_path << std::endl;

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        ParseShader(spirv_it.first, spirv32, bindings);

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        int set_num = GetSetNumByShaderType(spirv_it.first);
        if (descriptor_set_layouts.size() <= set_num)
        {
            descriptor_set_layouts.resize(set_num + 1);
        }

        VkDescriptorSetLayout& descriptor_set_layout = descriptor_set_layouts[set_num];
        if (vkCreateDescriptorSetLayout(m_context.m_device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        for (auto & binding : bindings)
        {
            descriptor_count[binding.descriptorType] += binding.descriptorCount;
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

    if (vkCreatePipelineLayout(m_context.m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to vkCreatePipelineLayout");
    }

    std::vector<VkDescriptorPoolSize> pool_sizes;

    for (auto& it : descriptor_count)
    {
        pool_sizes.emplace_back();
        VkDescriptorPoolSize& pool_size = pool_sizes.back();
        pool_size.type = it.first;
        pool_size.descriptorCount = it.second;
    }

  
    for (int i = 0; i < m_context.FrameCount; ++i)
    {
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = pool_sizes.size();
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = descriptor_set_layouts.size();

        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(m_context.m_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }


        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = descriptor_set_layouts.size();
        allocInfo.pSetLayouts = descriptor_set_layouts.data();

        m_descriptor_sets[i].resize(allocInfo.descriptorSetCount);
        if (vkAllocateDescriptorSets(m_context.m_device, &allocInfo, m_descriptor_sets[i].data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }
    }
}

void VKProgramApi::AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

 

    ShaderRef& shader_ref = m_shader_ref.find(type)->second;
    auto ref_res = shader_ref.resources[name];

 
    auto &res_type = shader_ref.compiler.get_type(ref_res.res.base_type_id);

    std::vector<VkWriteDescriptorSet> descriptorWrites;

    auto dim = res_type.image.dim;
    if (res_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
        dim = spv::Dim::DimBuffer;

    bool is_buffer = false;
    switch (dim)
    {
    case spv::Dim::Dim2D:
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = res.image.res;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = res.image.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = res.image.level_count;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView srv;
        if (vkCreateImageView(m_context.m_device, &viewInfo, nullptr, &srv) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = srv;
        
        descriptorWrites.emplace_back();
        auto& descriptorWrite = descriptorWrites.back();

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptor_sets.get()[GetSetNumByShaderType(type)];
        descriptorWrite.dstBinding = ref_res.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = ref_res.descriptor_type;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        break;
    }
    case spv::Dim::DimCube:
    {
        if (0)
        {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = res.image.res;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            viewInfo.format = res.image.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 6;

            VkImageView srv;
            if (vkCreateImageView(m_context.m_device, &viewInfo, nullptr, &srv) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = srv;

            descriptorWrites.emplace_back();
            auto& descriptorWrite = descriptorWrites.back();

            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_descriptor_sets.get()[GetSetNumByShaderType(type)];
            descriptorWrite.dstBinding = ref_res.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = ref_res.descriptor_type;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;
        }

        break;
    }
    case spv::Dim::DimBuffer:
    {
        is_buffer = true;
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = res.buffer.res;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        descriptorWrites.emplace_back();
        auto& descriptorWrite = descriptorWrites.back();

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptor_sets.get()[GetSetNumByShaderType(type)];
        descriptorWrite.dstBinding = ref_res.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = ref_res.descriptor_type;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        break;
    }
    default:
        throw std::runtime_error("failed!");
        break;
    }

    if (!is_buffer)
    {
        m_context.transitionImageLayout(res.image.res, {}, res.image.layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        res.image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (!descriptorWrites.empty())
        vkUpdateDescriptorSets(m_context.m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

}

void VKProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
}

void VKProgramApi::AttachCBuffer(ShaderType type, const std::string& name, UINT slot, BufferLayout & buffer_layout)
{
    m_cbv_buffer.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot, name),
        std::forward_as_tuple(m_context.CreateBuffer(BindFlag::kCbv, static_cast<uint32_t>(buffer_layout.GetBuffer().size()), 0)));
    m_cbv_layout.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot, name),
        std::forward_as_tuple(buffer_layout));
}

void VKProgramApi::AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc & desc)
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_context.m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    {
        VkWriteDescriptorSet descriptor_writes = {};

        VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = m_sampler;

        ShaderRef& shader_ref = m_shader_ref.find(type)->second;
        auto ref_res = shader_ref.resources["g_sampler"];

        // Populate with info about our sampler
        descriptor_writes = {};
        descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes.pNext = NULL;
        descriptor_writes.dstSet = m_descriptor_sets.get()[GetSetNumByShaderType(type)];
        descriptor_writes.descriptorCount = 1;
        descriptor_writes.descriptorType = ref_res.descriptor_type;
        descriptor_writes.pImageInfo = &samplerInfo;
        descriptor_writes.dstArrayElement = 0;
        descriptor_writes.dstBinding = ref_res.binding;

        vkUpdateDescriptorSets(m_context.m_device, 1, &descriptor_writes, 0, nullptr);
    }
}

void VKProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr & ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    m_context.transitionImageLayout(res.image.res, {}, res.image.layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    res.image.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkImageView rtv;
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = res.image.res;
    createInfo.format = res.image.format;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_context.m_device, &createInfo, nullptr, &rtv);
    m_rtv[slot] = rtv;
    m_rtv_size[slot] = res.image.size;
 

    VkAttachmentDescription& colorAttachment = m_color_attachments[slot];
    colorAttachment.format = res.image.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference& colorAttachmentRef = m_color_attachments_ref[slot];
    colorAttachmentRef.attachment = slot;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
}

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


void VKProgramApi::AttachDSV(const Resource::Ptr & ires)
{
    if (!ires)
        return;

    VKResource& res = static_cast<VKResource&>(*ires);

    m_context.transitionImageLayout(res.image.res, res.image.format, res.image.layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    res.image.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkImageView rtv;
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = res.image.res;
    createInfo.format = res.image.format;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent)
        createInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_context.m_device, &createInfo, nullptr, &rtv);
    m_rtv.back() = rtv;
    m_rtv_size.back() = res.image.size;

    auto& m_depth_attachment = m_color_attachments.back();
    m_depth_attachment.format = res.image.format;
    m_depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    m_depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    m_depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference& depthAttachmentReference = m_color_attachments_ref.back();
    depthAttachmentReference.attachment = m_color_attachments.size() - 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

void VKProgramApi::ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4])
{
}

void VKProgramApi::ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
}

void VKProgramApi::SetRasterizeState(const RasterizerDesc & desc)
{
}

void VKProgramApi::SetBlendState(const BlendDesc & desc)
{
}

void VKProgramApi::SetDepthStencilState(const DepthStencilDesc & desc)
{
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

    m_color_attachments.resize(m_num_rtv + 1);
    m_color_attachments_ref.resize(m_num_rtv + 1);
    m_rtv.resize(m_num_rtv + 1);
    m_rtv_size.resize(m_num_rtv + 1);
}

void VKProgramApi::AttachCBV(ShaderType type, const std::string& in_name, uint32_t slot, const Resource::Ptr & ires)
{
    if (!ires)
        return;

    std::string name = in_name;
    if (name == "$Globals")
        name = "_Global";

    VKResource& res = static_cast<VKResource&>(*ires);

    ShaderRef& shader_ref = m_shader_ref.find(type)->second;
    auto ref_res = shader_ref.resources[name];

    std::vector<VkWriteDescriptorSet> descriptorWrites;

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = res.buffer.res;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites.emplace_back();
    auto& descriptorWrite = descriptorWrites.back();

    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptor_sets.get()[GetSetNumByShaderType(type)];
    descriptorWrite.dstBinding = ref_res.binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = ref_res.descriptor_type;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    if (!descriptorWrites.empty())
        vkUpdateDescriptorSets(m_context.m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
