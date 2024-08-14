#include "Pipeline/VKPipeline.h"

#include "BindingSetLayout/VKBindingSetLayout.h"
#include "Device/VKDevice.h"

vk::ShaderStageFlagBits ExecutionModel2Bit(ShaderKind kind)
{
    switch (kind) {
    case ShaderKind::kVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderKind::kPixel:
        return vk::ShaderStageFlagBits::eFragment;
    case ShaderKind::kCompute:
        return vk::ShaderStageFlagBits::eCompute;
    case ShaderKind::kGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case ShaderKind::kAmplification:
        return vk::ShaderStageFlagBits::eTaskNV;
    case ShaderKind::kMesh:
        return vk::ShaderStageFlagBits::eMeshNV;
    case ShaderKind::kRayGeneration:
        return vk::ShaderStageFlagBits::eRaygenKHR;
    case ShaderKind::kIntersection:
        return vk::ShaderStageFlagBits::eIntersectionKHR;
    case ShaderKind::kAnyHit:
        return vk::ShaderStageFlagBits::eAnyHitKHR;
    case ShaderKind::kClosestHit:
        return vk::ShaderStageFlagBits::eClosestHitKHR;
    case ShaderKind::kMiss:
        return vk::ShaderStageFlagBits::eMissKHR;
    case ShaderKind::kCallable:
        return vk::ShaderStageFlagBits::eCallableKHR;
    default:
        assert(false);
        return {};
    }
}

VKPipeline::VKPipeline(VKDevice& device,
                       const std::shared_ptr<Program>& program,
                       const std::shared_ptr<BindingSetLayout>& layout)
    : m_device(device)
{
    decltype(auto) vk_layout = layout->As<VKBindingSetLayout>();
    m_pipeline_layout = vk_layout.GetPipelineLayout();

    decltype(auto) shaders = program->GetShaders();
    for (const auto& shader : shaders) {
        decltype(auto) blob = shader->GetBlob();
        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = blob.size();
        shader_module_info.pCode = (uint32_t*)blob.data();
        m_shader_modules.emplace_back(m_device.GetDevice().createShaderModuleUnique(shader_module_info));

        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) entry_points = reflection->GetEntryPoints();
        for (const auto& entry_point : entry_points) {
            m_shader_ids[shader->GetId(entry_point.name)] = m_shader_stage_create_info.size();
            decltype(auto) shader_stage_create_info = m_shader_stage_create_info.emplace_back();
            shader_stage_create_info.stage = ExecutionModel2Bit(entry_point.kind);
            shader_stage_create_info.module = m_shader_modules.back().get();
            decltype(auto) name = entry_point_names.emplace_back(entry_point.name);
            shader_stage_create_info.pName = name.c_str();
        }
    }
}

vk::Pipeline VKPipeline::GetPipeline() const
{
    return m_pipeline.get();
}

vk::PipelineLayout VKPipeline::GetPipelineLayout() const
{
    return m_pipeline_layout;
}

std::vector<uint8_t> VKPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const
{
    return {};
}
