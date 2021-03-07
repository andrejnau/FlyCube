#pragma once
#include "Pipeline/VKPipeline.h"
#include <Instance/BaseTypes.h>
#include <vulkan/vulkan.hpp>
#include <RenderPass/VKRenderPass.h>
#include <ShaderReflection/ShaderReflection.h>

class VKDevice;

class VKComputePipeline : public VKPipeline
{
public:
    VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;

private:
    ComputePipelineDesc m_desc;
};
