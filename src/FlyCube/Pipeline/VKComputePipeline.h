#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/VKPipeline.h"
#include "RenderPass/VKRenderPass.h"
#include "ShaderReflection/ShaderReflection.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKComputePipeline : public VKPipeline {
public:
    VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;

private:
    ComputePipelineDesc m_desc;
};
