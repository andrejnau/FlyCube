#pragma once
#include <Swapchain/Swapchain.h>
#include <memory>
#include <GLFW/glfw3.h>

class Device
{
public:
    virtual ~Device() = default;
    virtual std::unique_ptr<Swapchain> CreateSwapchain(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count) = 0;
};
