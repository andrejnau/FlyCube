#pragma once
#include "Resource/ResourceBase.h"

#include <vulkan/vulkan.hpp>

class VKResource : public ResourceBase {
public:
    virtual vk::Image GetImage() const;
    virtual vk::Buffer GetBuffer() const;
    virtual vk::Sampler GetSampler() const;
    virtual vk::AccelerationStructureKHR GetAccelerationStructure() const;
};
