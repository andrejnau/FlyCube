#pragma once
#include "Fence/Fence.h"
#include "Instance/QueryInterface.h"
#include "Resource/Resource.h"

#include <gli/format.hpp>

using WindowHandle = void*;

class Swapchain : public QueryInterface {
public:
    virtual ~Swapchain() = default;
    virtual gli::format GetFormat() const = 0;
    virtual std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) = 0;
    virtual uint32_t NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value) = 0;
    virtual void Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value) = 0;
};
