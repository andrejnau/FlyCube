#pragma once
#include <View/View.h>
#include <View/ViewDesc.h>

class VKResource;

class VKView : public View
{
public:
    VKView(VKResource& resource, const ViewDesc& view_desc);

private:
    VKResource& m_resource;
};
