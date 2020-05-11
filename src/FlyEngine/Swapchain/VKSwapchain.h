#pragma once
#include "Swapchain.h"
#include <Utilities/Vulkan.h>
#include <Resource/VKResource.h>
#include <memory>

class VKDevice;

class VKSwapchain
    : public Swapchain
    , public VKResource::Destroyer
{
public:
    VKSwapchain(VKDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count);
    Resource::Ptr GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Semaphore>& semaphore) override;
    void Present(const std::shared_ptr<Semaphore>& semaphore) override;

    void QueryOnDelete(VKResource res) override;

private:
    VKDevice& m_device;
    vk::UniqueSurfaceKHR m_surface;
    vk::Format m_swapchain_color_format = vk::Format::eB8G8R8Unorm;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<std::shared_ptr<VKResource>> m_back_buffers;
    uint32_t m_frame_index = 0;
};
