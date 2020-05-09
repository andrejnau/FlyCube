#pragma once
#include <GLFW/glfw3.h>
#include <Resource/Resource.h>

class Swapchain
{
public:
    virtual ~Swapchain() = default;
    virtual uint32_t GetCurrentBackBufferIndex() = 0;
    virtual Resource::Ptr GetBackBuffer(uint32_t buffer) = 0;
    virtual void Present() = 0;
};
