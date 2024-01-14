#pragma once

#include "AppLoop/AppSize.h"

#include <string_view>

class AppRenderer {
public:
    virtual ~AppRenderer() = default;
    virtual void Init(const AppSize& app_size, void* window) = 0;
    virtual void Resize(const AppSize& app_size, void* window) {}
    virtual void Render() = 0;
    virtual std::string_view GetTitle() const = 0;
};
