#pragma once
#include "ProgramBase.h"
#include <Shader/Shader.h>
#include <Utilities/Vulkan.h>
#include <vector>
#include <map>
#include <set>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKDevice;
class SpirvShader;

class VKProgram : public ProgramBase
{
public:
    VKProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);
    bool HasBinding(ShaderType shader, ViewType type, std::string name) const override;
    std::shared_ptr<BindingSet> CreateBindingSetImpl(const BindingsKey& bindings) override;

    const std::vector<std::shared_ptr<SpirvShader>>& GetShaders() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    void ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary,
                     std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                     std::vector<vk::DescriptorBindingFlagsEXT>& bindings_flags);

    VKDevice& m_device;
    std::vector<std::shared_ptr<SpirvShader>> m_shaders;

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
    };

    std::map<ShaderType, ShaderRef> m_shader_ref;
    std::vector<std::map<vk::DescriptorType, size_t>> m_descriptor_count_by_set;
};
