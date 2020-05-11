#pragma once
#include <View/View.h>
#include <View/ViewDesc.h>

class DXResource;

class DXView : public View
{
public:
    DXView(DXResource& resource, const ViewDesc& view_desc);

private:
    DXResource& m_resource;
};
