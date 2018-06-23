#pragma once

#include <vulkan/vulkan.h>
#include "Context/Resource.h"

class VKResource : public Resource
{
public:
    using Ptr = std::shared_ptr<VKResource>;
    VkBuffer buf;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    uint32_t buffer_size = 0;

    virtual void SetName(const std::string& name) override
    {
    }
};
