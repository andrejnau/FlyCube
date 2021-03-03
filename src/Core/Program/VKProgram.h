#pragma once
#include "ProgramBase.h"
#include <Shader/Shader.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <map>
#include <set>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

class VKDevice;
class Shader;

class VKProgram : public ProgramBase
{
public:
    VKProgram(VKDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);
    bool HasBinding(const BindKey& bind_key) const override;
    std::shared_ptr<BindingSet> CreateBindingSetImpl(const std::vector<BindingDesc>& bindings) override;

    const std::vector<std::shared_ptr<Shader>>& GetShaders() const;
    vk::PipelineLayout GetPipelineLayout() const;

private:
    void ParseShader(ShaderType shader_type, const std::vector<uint8_t>& spirv_binary,
                     std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>>& bindings,
                     std::map<uint32_t, std::vector<vk::DescriptorBindingFlags>>& bindings_flags);

    VKDevice& m_device;
    std::vector<std::shared_ptr<Shader>> m_shaders;

    std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts;
    vk::UniquePipelineLayout m_pipeline_layout;

    struct ResourceDesc
    {
        vk::DescriptorType descriptor_type;
        uint32_t binding;
    };

    std::map<BindKey, ResourceDesc> m_resources;

    std::map<uint32_t, vk::DescriptorType> m_bindless_type;
    std::vector<std::map<vk::DescriptorType, size_t>> m_descriptor_count_by_set;
};
