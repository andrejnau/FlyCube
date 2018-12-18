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

    struct Settings
    {
        size_t level = 0;
        bool operator<(const Settings& oth) const
        {
            return level < oth.level;
        }
    };

    ViewId GetCustomViewId(size_t level)
    {
        Settings custom_view = { level };
        auto it = m_rtv_view_settings.find(custom_view);
        if (it == m_rtv_view_settings.end())
        {
            m_rtv_view_settings_data.emplace_back(custom_view);
            it = m_rtv_view_settings.emplace(std::piecewise_construct, std::forward_as_tuple(custom_view), std::forward_as_tuple(ViewId{ m_rtv_view_settings_data.size() })).first;
        }
        return it->second;
    }

    const Settings& GetCustomViewSetting(ViewId id) const
    {
        if (id.value - 1 >= m_rtv_view_settings_data.size())
        {
            static Settings empty;
            return empty;
        }
        return m_rtv_view_settings_data[id.value - 1];
    }

private:
    std::map<BindKey, View::Ptr> views;
    std::map<Settings, ViewId> m_rtv_view_settings;
    std::vector<Settings> m_rtv_view_settings_data;
};
