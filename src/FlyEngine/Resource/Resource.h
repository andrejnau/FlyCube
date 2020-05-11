#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "Context/BaseTypes.h"
#include <gli/gli.hpp>

struct ViewDesc
{
    ViewDesc() = default;
    ViewDesc(size_t level, size_t count = -1)
        : level(level)
        , count(count)
    {
    }

    size_t level = 0;
    size_t count = -1;
    bool operator<(const ViewDesc& oth) const
    {
        return level < oth.level;
    }
};

class Resource
{
public:
    virtual ~Resource() = default;
    virtual void SetName(const std::string& name) = 0;

    /*View::Ptr& GetView(const BindKey& view_key, const ViewDesc& view_desc)
    {
        std::pair<BindKey, ViewDesc> key = { view_key, view_desc };
        auto it = views.find(key);
        if (it == views.end())
            it = views.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(View::Ptr())).first;
        return it->second;
    }*/

private:
   // std::map<std::pair<BindKey, ViewDesc>, View::Ptr> views;
};

struct BufferDesc
{
    std::shared_ptr<Resource> res;
    gli::format format = gli::format::FORMAT_UNDEFINED;
    uint32_t count = 0;
    uint32_t offset = 0;
};
