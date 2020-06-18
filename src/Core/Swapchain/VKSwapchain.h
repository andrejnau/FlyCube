#pragma once
#include "Swapchain.h"
#include <Utilities/Vulkan.h>
#include <Resource/VKResource.h>
#include <memory>
#include <vector>

class VKDevice;

class VKSwapchain
    : public Swapchain
{
public:
    VKSwapchain(VKDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync);
    gli::format GetFormat() const override;
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value) override;
    void Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value) override;

private:
    VKDevice& m_device;
    vk::UniqueSurfaceKHR m_surface;
    vk::Format m_swapchain_color_format = vk::Format::eB8G8R8Unorm;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<std::shared_ptr<Resource>> m_back_buffers;
    uint32_t m_frame_index = 0;
    vk::UniqueSemaphore m_image_available_semaphore;
    vk::UniqueSemaphore m_rendering_finished_semaphore;
};
