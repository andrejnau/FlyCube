#pragma once
#include <Instance/QueryInterface.h>
#include <Resource/Resource.h>
#include <Semaphore/Semaphore.h>
#include <GLFW/glfw3.h>

class Swapchain : public QueryInterface
{
public:
    virtual ~Swapchain() = default;
    virtual std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) = 0;
    virtual uint32_t NextImage(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void Present(const std::shared_ptr<Semaphore>& semaphore) = 0;
};
