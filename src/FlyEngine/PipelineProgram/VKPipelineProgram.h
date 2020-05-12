#pragma once
#include "PipelineProgram.h"
#include <Shader/Shader.h>
#include <Utilities/Vulkan.h>
#include <vector>
#include <map>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKDevice;
class SpirvShader;

class VKPipelineProgram : public PipelineProgram
{
public:
    VKPipelineProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);

private:
    void ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary, std::vector<vk::DescriptorSetLayoutBinding>& bindings);

    VKDevice& m_device;
    std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts;
    vk::UniquePipelineLayout m_pipeline_layout;

    struct ShaderRef
    {
        ShaderRef(const std::vector<uint32_t>& spirv_binary)
            : compiler(std::move(spirv_binary))
        {
        }

        spirv_cross::CompilerHLSL compiler;

        struct ResourceRef
        {
            spirv_cross::Resource res;
            vk::DescriptorType descriptor_type;
            uint32_t binding;
        };

        std::map<std::string, ResourceRef> resources;

        spirv_cross::SmallVector<spirv_cross::EntryPoint> entries;
    };

    std::map<ShaderType, ShaderRef> m_shader_ref;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfo;
    std::map<ShaderType, vk::UniqueShaderModule> m_shader_modules;
    std::vector<std::shared_ptr<SpirvShader>> m_shaders;
};
