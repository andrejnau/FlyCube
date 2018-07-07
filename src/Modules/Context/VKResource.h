#pragma once

#include <vulkan/vulkan.h>
#include "Context/Resource.h"

class VKResource : public Resource
{
public:
    using Ptr = std::shared_ptr<VKResource>;
    VkBuffer buf;
    VkImage image;
    VkImage tmp_image;
    VkDeviceMemory tmp_image_memory;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    uint32_t buffer_size = 0;

    VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDeviceMemory image_memory;

    VkExtent2D size = {};
    VkFormat format;
    size_t levelCount = 0;

    virtual void SetName(const std::string& name) override
    {
    }
};
