#include "Pipeline/VKRayTracingPipeline.h"

#include "Adapter/VKAdapter.h"
#include "BindingSetLayout/VKBindingSetLayout.h"
#include "Device/VKDevice.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "Shader/Shader.h"

#include <map>

VKRayTracingPipeline::VKRayTracingPipeline(VKDevice& device, const RayTracingPipelineDesc& desc)
    : VKPipeline(device, desc.shaders, desc.layout)
    , desc_(desc)
{
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups(desc_.groups.size());

    auto get = [&](uint64_t id) -> uint32_t {
        auto it = shader_ids_.find(id);
        if (it == shader_ids_.end()) {
            return VK_SHADER_UNUSED_KHR;
        }
        return it->second;
    };

    for (size_t i = 0; i < desc_.groups.size(); ++i) {
        decltype(auto) group = groups[i];
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        switch (desc_.groups[i].type) {
        case RayTracingShaderGroupType::kGeneral:
            group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            group.generalShader = get(desc_.groups[i].general);
            break;
        case RayTracingShaderGroupType::kTrianglesHitGroup:
            group.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            group.closestHitShader = get(desc_.groups[i].closest_hit);
            group.anyHitShader = get(desc_.groups[i].any_hit);
            break;
        case RayTracingShaderGroupType::kProceduralHitGroup:
            group.type = vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
            group.intersectionShader = get(desc_.groups[i].intersection);
            break;
        }
    }

    vk::RayTracingPipelineCreateInfoKHR ray_pipeline_info{};
    ray_pipeline_info.stageCount = static_cast<uint32_t>(shader_stage_create_info_.size());
    ray_pipeline_info.pStages = shader_stage_create_info_.data();
    ray_pipeline_info.groupCount = static_cast<uint32_t>(groups.size());
    ray_pipeline_info.pGroups = groups.data();
    ray_pipeline_info.maxPipelineRayRecursionDepth = 1;
    ray_pipeline_info.layout = pipeline_layout_;

    pipeline_ = device_.GetDevice().createRayTracingPipelineKHRUnique({}, {}, ray_pipeline_info).value;
}

PipelineType VKRayTracingPipeline::GetPipelineType() const
{
    return PipelineType::kRayTracing;
}

std::vector<uint8_t> VKRayTracingPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group,
                                                                           uint32_t group_count) const
{
    std::vector<uint8_t> shader_handles_storage(group_count * device_.GetShaderGroupHandleSize());
    std::ignore = device_.GetDevice().getRayTracingShaderGroupHandlesKHR(
        pipeline_.get(), first_group, group_count, shader_handles_storage.size(), shader_handles_storage.data());
    return shader_handles_storage;
}
