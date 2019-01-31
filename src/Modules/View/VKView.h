#pragma once

#include <map>
#include <algorithm>
#include <stdint.h>
#include <Resource/VKResource.h>
#include <View/View.h>

#include <Context/VKContext.h>
#include <vulkan/vulkan.h>

class VKView : public View
{
public:
    using Ptr = std::shared_ptr<VKView>;
    VkImageView srv;
    VkImageView om;
};
