#include "Pipeline/VKComputePipeline.h"

#include "Device/VKDevice.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "Program/ProgramBase.h"
#include "Shader/Shader.h"

#include <map>

VKComputePipeline::VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc)
    : VKPipeline(device, desc.program, desc.layout)
    , m_desc(desc)
{
    vk::ComputePipelineCreateInfo pipeline_info = {};
    assert(m_shader_stage_create_info.size() == 1);
    pipeline_info.stage = m_shader_stage_create_info.front();
    pipeline_info.layout = m_pipeline_layout;
    m_pipeline = m_device.GetDevice().createComputePipelineUnique({}, pipeline_info).value;
}

PipelineType VKComputePipeline::GetPipelineType() const
{
    return PipelineType::kCompute;
}
