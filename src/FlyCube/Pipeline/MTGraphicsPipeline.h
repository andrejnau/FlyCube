#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/MTPipeline.h"

#import <Metal/Metal.h>

class MTDevice;

class MTGraphicsPipeline : public MTPipeline {
public:
    MTGraphicsPipeline(MTDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;
    std::shared_ptr<Shader> GetShader(ShaderType type) const override;

    id<MTLRenderPipelineState> GetPipeline();
    id<MTLDepthStencilState> GetDepthStencil();

    const GraphicsPipelineDesc& GetDesc() const;
    const MTLSize& GetAmplificationNumthreads() const;
    const MTLSize& GetMeshNumthreads() const;

private:
    template <bool is_mesh_pipeline>
    void CreatePipeline();
    MTLVertexDescriptor* GetVertexDescriptor(const std::shared_ptr<Shader>& shader);

    MTDevice& device_;
    GraphicsPipelineDesc desc_;
    std::map<ShaderType, std::shared_ptr<Shader>> shader_by_type_;
    id<MTLRenderPipelineState> pipeline_ = nullptr;
    id<MTLDepthStencilState> depth_stencil_ = nullptr;

    MTLSize amplification_numthreads_ = {};
    MTLSize mesh_numthreads_ = {};
};
