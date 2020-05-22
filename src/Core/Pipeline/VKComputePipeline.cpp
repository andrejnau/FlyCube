#include "Pipeline/VKComputePipeline.h"
#include <Device/VKDevice.h>
#include <Program/VKProgram.h>
#include <Shader/SpirvShader.h>
#include <map>

static vk::ShaderStageFlagBits ExecutionModel2Bit(spv::ExecutionModel model)
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
    }
    assert(false);
    return {};
}

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
