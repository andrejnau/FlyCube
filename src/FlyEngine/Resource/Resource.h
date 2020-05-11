#pragma once
#include <View/View.h>
#include <View/ViewDesc.h>
#include <memory>
#include <string>

class Resource
{
public:
    virtual ~Resource() = default;
    virtual void SetName(const std::string& name) = 0;
    virtual std::shared_ptr<View> CreateView(const ViewDesc& view_desc) = 0;
};
