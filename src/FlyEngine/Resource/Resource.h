#pragma once
#include <View/View.h>
#include <memory>
#include <string>
#include <gli/gli.hpp>

class Resource
{
public:
    virtual ~Resource() = default;
    virtual gli::format GetFormat() const = 0;
    virtual void SetName(const std::string& name) = 0;
};
