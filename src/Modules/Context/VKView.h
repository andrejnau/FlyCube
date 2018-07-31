#pragma once

#include <map>
#include <algorithm>
#include <stdint.h>
#include <Context/VKResource.h>
#include <Context/View.h>

#include <Context/VKContext.h>
#include <vulkan/vulkan.h>

class VKView : public View
{
public:
    VkImageView srv;
    VkImageView rtv;
    VkImageView dsv;
};
