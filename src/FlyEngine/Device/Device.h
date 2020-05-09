#pragma once
#include <Swapchain/Swapchain.h>
#include <CommandList/CommandList.h>
#include <Fence/Fence.h>
#include <memory>
#include <vector>
#include <GLFW/glfw3.h>

class Device
{
public:
    virtual ~Device() = default;
    virtual std::unique_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) = 0;
    virtual std::unique_ptr<CommandList> CreateCommandList() = 0;
    virtual std::unique_ptr<Fence> CreateFence() = 0;
    virtual void ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& command_lists, const std::unique_ptr<Fence>& fence) = 0;
};
