#pragma once
#include <GLFW/glfw3.h>
#include <Resource/Resource.h>
#include <Semaphore/Semaphore.h>

class Swapchain
{
public:
    virtual ~Swapchain() = default;
    virtual Resource::Ptr GetBackBuffer(uint32_t buffer) = 0;
    virtual uint32_t NextImage(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void Present(const std::shared_ptr<Semaphore>& semaphore) = 0;
};
