#pragma once
#include "Pipeline/Pipeline.h"
#include <Instance/BaseTypes.h>
#import <Metal/Metal.h>

class MTDevice;
class Shader;

class MTGraphicsPipeline : public Pipeline
{
public:
    MTGraphicsPipeline(MTDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;

private:
    MTLVertexDescriptor* GetVertexDescriptor(const std::shared_ptr<Shader>& shader);

    MTDevice& m_device;
    GraphicsPipelineDesc m_desc;
    id<MTLRenderPipelineState> m_pipeline;
};
