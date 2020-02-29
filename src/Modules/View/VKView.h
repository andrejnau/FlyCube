#pragma once

#include <map>
#include <algorithm>
#include <stdint.h>
#include <Resource/VKResource.h>
#include <View/View.h>

#include <Context/VKContext.h>

class VKView : public View
{
public:
    using Ptr = std::shared_ptr<VKView>;
    vk::UniqueImageView srv;
    vk::UniqueImageView om;
};
