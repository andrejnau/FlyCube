#pragma once
#include "Pipeline/VKPipeline.h"
#include <Instance/BaseTypes.h>
#include <vulkan/vulkan.hpp>
#include <RenderPass/VKRenderPass.h>
#include <ShaderReflection/ShaderReflection.h>

class VKDevice;

class VKRayTracingPipeline : public VKPipeline
{
public:
    VKRayTracingPipeline(VKDevice& device, const ComputePipelineDesc& desc);
    PipelineType GetPipelineType() const override;

    vk::Buffer GetShaderBindingTable() const;

private:
    ComputePipelineDesc m_desc;
    vk::UniqueBuffer m_shader_binding_table;
    vk::UniqueDeviceMemory m_shader_binding_table_memory;
};
