#include "VKProgramApi.h"

VKProgramApi::VKProgramApi(VKContext& context)
    : m_context(context)
{
}

void VKProgramApi::SetMaxEvents(size_t)
{
}

void VKProgramApi::LinkProgram()
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
    for (auto & shader : m_shaders_info)
    {
        shaderStageCreateInfo.emplace_back();

        switch (shader.first)
        {
        case ShaderType::kVertex:
            shaderStageCreateInfo.back().stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case ShaderType::kPixel:
            shaderStageCreateInfo.back().stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        case ShaderType::kGeometry:
            shaderStageCreateInfo.back().stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        case ShaderType::kCompute:
            shaderStageCreateInfo.back().stage = VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        }

        shaderStageCreateInfo.back().sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.back().module = m_shaders[shader.first];
        shaderStageCreateInfo.back().pName = shader.second.c_str();
        shaderStageCreateInfo.back().pSpecializationInfo = NULL;
    }

    std::vector<VkVertexInputBindingDescription> binding_desc;
    std::vector<VkVertexInputAttributeDescription> attribute_desc;
    if (m_spirv.count(ShaderType::kVertex))
    {
        auto& spirv = m_spirv[ShaderType::kVertex];
        assert(spirv.size() % 4 == 0);
        std::vector<uint32_t> spirv32((uint32_t*)spirv.data(), ((uint32_t*)spirv.data()) + spirv.size() / 4);
        CreateInputLayout(spirv32, binding_desc, attribute_desc);
    }

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
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = NULL;    // Not setting any bindings!
    layoutCreateInfo.pushConstantRangeCount = 0;
    layoutCreateInfo.pPushConstantRanges = NULL;

    VkPipelineLayout pipelineLayout;
    auto result = vkCreatePipelineLayout(m_context.m_device, &layoutCreateInfo, NULL, &pipelineLayout);

    //
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;

    VkRenderPass renderPass;
    auto re2s = vkCreateRenderPass(m_context.m_device, &renderPassInfo, nullptr, &renderPass);
    //

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStageCreateInfo.size();
    pipelineInfo.pStages = shaderStageCreateInfo.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1; // Optional

   // auto res = vkCreateGraphicsPipelines(m_context.m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    int b = 0;
}

void VKProgramApi::UseProgram()
{
}

void VKProgramApi::ApplyBindings()
{
    vkCmdBindPipeline(m_context.m_cmd_bufs[m_context.GetFrameIndex()], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
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

std::vector<uint8_t> hlsl2spirv(const ShaderBase& shader)
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

 
}

void VKProgramApi::AttachSRV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
}

void VKProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
}

void VKProgramApi::AttachCBuffer(ShaderType type, UINT slot, BufferLayout & buffer_layout)
{
}

void VKProgramApi::AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc & desc)
{
}

void VKProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr & ires)
{
}

void VKProgramApi::AttachDSV(const Resource::Ptr & ires)
{
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

#include <spirv_cross.hpp>
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include <vector>
#include <utility>

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
            else if (type.vecsize == 2)
                attribute.format = VK_FORMAT_R32G32_SINT;
            else if (type.vecsize == 3)
                attribute.format = VK_FORMAT_R32G32B32_SINT;
            else if (type.vecsize == 4)
                attribute.format = VK_FORMAT_R32G32B32A32_SINT;
        }
    }
}
