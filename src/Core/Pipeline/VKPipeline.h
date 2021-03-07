#pragma once
#include "Pipeline/Pipeline.h"
#include <vulkan/vulkan.hpp>

class VKPipeline : public Pipeline
{
public:
    virtual ~VKPipeline() = default;
    virtual vk::Pipeline GetPipeline() const = 0;
    virtual vk::PipelineLayout GetPipelineLayout() const = 0;
    virtual vk::PipelineBindPoint GetPipelineBindPoint() const = 0;
};
