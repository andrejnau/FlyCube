#pragma once

#include <wrl.h>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include <Utilities/FileUtility.h>
#include "Context/View.h"
#include "Context/BaseTypes.h"

using namespace Microsoft::WRL;

class Resource
{
public:
    virtual ~Resource() = default;
    virtual void SetName(const std::string& name) = 0;
    using Ptr = std::shared_ptr<Resource>;

    View::Ptr& GetView(const BindKey& bind_key)
    {
        auto it = views.find(bind_key);
        if (it == views.end())
        {
            it = views.emplace(std::piecewise_construct, std::forward_as_tuple(bind_key), std::forward_as_tuple(View::Ptr())).first;
        }
        return it->second;
    }

private:
    std::map<BindKey, View::Ptr> views;
};
