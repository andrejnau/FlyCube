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
    void CreateInputLayout(std::vector<vk::VertexInputBindingDescription>& binding_desc,
                           std::vector<vk::VertexInputAttributeDescription>& attribute_desc);

    GraphicsPipelineDesc m_desc;
    std::vector<vk::VertexInputBindingDescription> m_binding_desc;
    std::vector<vk::VertexInputAttributeDescription> m_attribute_desc;
};
