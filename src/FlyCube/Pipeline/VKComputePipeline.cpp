#include "Pipeline/VKComputePipeline.h"

#include "Device/VKDevice.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "Shader/Shader.h"

#include <map>

VKComputePipeline::VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc)
    : VKPipeline(device, { desc.shader }, desc.layout)
    , desc_(desc)
{
    vk::ComputePipelineCreateInfo pipeline_info = {};
    assert(shader_stage_create_info_.size() == 1);
    pipeline_info.stage = shader_stage_create_info_.front();
    pipeline_info.layout = pipeline_layout_;
    pipeline_ = device_.GetDevice().createComputePipelineUnique({}, pipeline_info).value;
}

PipelineType VKComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}
