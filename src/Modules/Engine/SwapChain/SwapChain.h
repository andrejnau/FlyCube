#pragma once

#include <memory>

class SwapChain
{
public:
    using Ptr = std::shared_ptr<SwapChain>;
    virtual void Present() = 0;
};
