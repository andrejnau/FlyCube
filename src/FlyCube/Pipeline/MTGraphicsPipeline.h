#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/MTPipeline.h"

#import <Metal/Metal.h>

class MTDevice;
class Shader;

class MTGraphicsPipeline : public MTPipeline {
public:
    MTGraphicsPipeline(MTDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;
    std::shared_ptr<Program> GetProgram() const override;

    id<MTLRenderPipelineState> GetPipeline();
    id<MTLDepthStencilState> GetDepthStencil();

    const GraphicsPipelineDesc& GetDesc() const;
    const MTLSize& GetAmplificationNumthreads() const;
    const MTLSize& GetMeshNumthreads() const;

private:
    template <bool is_mesh_pipeline>
    void CreatePipeline();
    MTLVertexDescriptor* GetVertexDescriptor(const std::shared_ptr<Shader>& shader);

    MTDevice& m_device;
    GraphicsPipelineDesc m_desc;
    id<MTLRenderPipelineState> m_pipeline;
    id<MTLDepthStencilState> m_depth_stencil;

    MTLSize m_amplification_numthreads = {};
    MTLSize m_mesh_numthreads = {};
};
