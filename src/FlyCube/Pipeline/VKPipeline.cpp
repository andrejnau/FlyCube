#include "Pipeline/VKPipeline.h"

#include "BindingSetLayout/VKBindingSetLayout.h"
#include "Device/VKDevice.h"
#include "Utilities/NotReached.h"

namespace {

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
        return vk::ShaderStageFlagBits::eTaskEXT;
    case ShaderKind::kMesh:
        return vk::ShaderStageFlagBits::eMeshEXT;
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
        NOTREACHED();
    }
}

} // namespace

VKPipeline::VKPipeline(VKDevice& device,
                       const std::vector<std::shared_ptr<Shader>>& shaders,
                       const std::shared_ptr<BindingSetLayout>& layout)
    : device_(device)
{
    decltype(auto) vk_layout = layout->As<VKBindingSetLayout>();
    pipeline_layout_ = vk_layout.GetPipelineLayout();

    for (const auto& shader : shaders) {
        decltype(auto) blob = shader->GetBlob();
        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = blob.size();
        shader_module_info.pCode = (uint32_t*)blob.data();
        shader_modules_.emplace_back(device_.GetDevice().createShaderModuleUnique(shader_module_info));

        decltype(auto) reflection = shader->GetReflection();
        decltype(auto) entry_points = reflection->GetEntryPoints();
        for (const auto& entry_point : entry_points) {
            shader_ids_[shader->GetId(entry_point.name)] = shader_stage_create_info_.size();
            decltype(auto) shader_stage_create_info = shader_stage_create_info_.emplace_back();
            shader_stage_create_info.stage = ExecutionModel2Bit(entry_point.kind);
            shader_stage_create_info.module = shader_modules_.back().get();
            decltype(auto) name = entry_point_names.emplace_back(entry_point.name);
            shader_stage_create_info.pName = name.c_str();
        }
    }
}

vk::Pipeline VKPipeline::GetPipeline() const
{
    return pipeline_.get();
}

vk::PipelineLayout VKPipeline::GetPipelineLayout() const
{
    return pipeline_layout_;
}

std::vector<uint8_t> VKPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const
{
    return {};
}
