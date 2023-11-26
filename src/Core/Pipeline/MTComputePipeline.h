#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/MTPipeline.h"

#import <Metal/Metal.h>

class MTDevice;
class Shader;

class MTComputePipeline : public MTPipeline {
public:
    MTComputePipeline(MTDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;
    std::shared_ptr<Program> GetProgram() const override;

    id<MTLComputePipelineState> GetPipeline();

    const ComputePipelineDesc& GetDesc() const;
    const MTLSize GetNumthreads() const;

private:
    MTLVertexDescriptor* GetVertexDescriptor(const std::shared_ptr<Shader>& shader);

    MTDevice& m_device;
    ComputePipelineDesc m_desc;
    id<MTLComputePipelineState> m_pipeline;
    MTLSize m_numthreads = {};
};
