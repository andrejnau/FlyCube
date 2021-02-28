#include "Pipeline/VKComputePipeline.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include <Device/VKDevice.h>
#include <Program/VKProgram.h>
#include <Shader/Shader.h>
#include <map>

VKComputePipeline::VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
{
    decltype(auto) vk_program = desc.program->As<VKProgram>();
    auto shaders = vk_program.GetShaders();
    for (auto& shader : shaders)
    {
        ShaderType shader_type = shader->GetType();
        auto blob = shader->GetBlob();

        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = blob.size();
        shader_module_info.pCode = (uint32_t*)blob.data();

        m_shader_modules[shader_type] = m_device.GetDevice().createShaderModuleUnique(shader_module_info);

        spirv_cross::CompilerHLSL compiler((uint32_t*)blob.data(), blob.size() / sizeof(uint32_t));
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
    assert(m_shader_stage_create_info.size() == 1);
    CreateComputePipeLine();
}

PipelineType VKComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}

vk::Pipeline VKComputePipeline::GetPipeline() const
{
    return m_pipeline.get();
}

void VKComputePipeline::CreateComputePipeLine()
{
    decltype(auto) vk_program = m_desc.program->As<VKProgram>();
    vk::ComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.stage = *m_shader_stage_create_info.data();
    pipeline_info.layout = vk_program.GetPipelineLayout();
    m_pipeline = m_device.GetDevice().createComputePipelineUnique({}, pipeline_info);
}
