#pragma once
#include "View/View.h"
#include "View/ViewDesc.h"
#include <Resource/Resource.h>

class VKDevice;

class VKView : public View
{
public:
    VKView(VKDevice& device, const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc);
};
