#pragma once
#include <Swapchain/Swapchain.h>
#include <CommandList/CommandList.h>
#include <Fence/Fence.h>
#include <Semaphore/Semaphore.h>
#include <memory>
#include <vector>
#include <GLFW/glfw3.h>

class Device
{
public:
    virtual ~Device() = default;
    virtual std::shared_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) = 0;
    virtual std::shared_ptr<CommandList> CreateCommandList() = 0;
    virtual std::shared_ptr<Fence> CreateFence() = 0;
    virtual std::shared_ptr<Semaphore> CreateGPUSemaphore() = 0;
    virtual void Wait(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void Signal(const std::shared_ptr<Semaphore>& semaphore) = 0;
    virtual void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::shared_ptr<Fence>& fence) = 0;
};
