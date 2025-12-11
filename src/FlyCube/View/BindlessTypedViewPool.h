#pragma once
#include "Instance/QueryInterface.h"

#include <cstdint>
#include <memory>

class View;

class BindlessTypedViewPool : public QueryInterface {
public:
    virtual ~BindlessTypedViewPool() = default;
    virtual uint32_t GetBaseDescriptorId() const = 0;
    virtual uint32_t GetViewCount() const = 0;
    virtual void WriteView(uint32_t index, const std::shared_ptr<View>& view) = 0;
};
