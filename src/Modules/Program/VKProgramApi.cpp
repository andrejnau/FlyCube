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
    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = NULL;    // Not setting any bindings!
    layoutCreateInfo.pushConstantRangeCount = 0;
    layoutCreateInfo.pPushConstantRanges = NULL;

    VkPipelineLayout pipelineLayout;
    auto result = vkCreatePipelineLayout(m_context.m_device, &layoutCreateInfo, NULL,
        &pipelineLayout);

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
}

void VKProgramApi::UseProgram()
{
}

void VKProgramApi::ApplyBindings()
{
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
