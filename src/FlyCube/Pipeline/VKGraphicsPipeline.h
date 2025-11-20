#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/VKPipeline.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKGraphicsPipeline : public VKPipeline {
public:
    VKGraphicsPipeline(VKDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;

    const GraphicsPipelineDesc& GetDesc() const;

private:
    void CreateInputLayout(const std::shared_ptr<Shader>& shader);

    GraphicsPipelineDesc desc_;
    std::vector<vk::VertexInputBindingDescription> binding_desc_;
    std::vector<vk::VertexInputAttributeDescription> attribute_desc_;
};
