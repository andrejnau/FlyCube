#include "Pipeline/SWGraphicsPipeline.h"
#include <Device/SWDevice.h>
#include <map>

SWGraphicsPipeline::SWGraphicsPipeline(SWDevice& device, const GraphicsPipelineDesc& desc)
    : m_desc(desc)
{
}

PipelineType SWGraphicsPipeline::GetPipelineType() const
{
    return PipelineType::kGraphics;
}

std::vector<uint8_t> SWGraphicsPipeline::GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const
{
    return {};
}
