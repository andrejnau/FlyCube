#pragma once

#include "ApiType/ApiType.h"
#include "Instance/BaseTypes.h"

#include <string>
#include <string_view>

class AppRenderer {
public:
    virtual ~AppRenderer() = default;
    virtual void Init(const NativeSurface& surface, uint32_t width, uint32_t height) = 0;
    virtual void Resize(const NativeSurface& surface, uint32_t width, uint32_t height) {}
    virtual void Render() = 0;
    virtual std::string_view GetTitle() const = 0;
    virtual const std::string& GetGpuName() const = 0;
    virtual ApiType GetApiType() const = 0;
};
