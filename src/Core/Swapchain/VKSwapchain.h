#pragma once
#include "Resource/VKResource.h"
#include "Swapchain.h"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

class VKDevice;
class VKCommandQueue;
class CommandList;
class Fence;

class VKSwapchain : public Swapchain {
public:
    VKSwapchain(VKCommandQueue& command_queue,
                WindowHandle window,
                uint32_t width,
                uint32_t height,
                uint32_t frame_count,
                bool vsync);
    ~VKSwapchain();
    gli::format GetFormat() const override;
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value) override;
    void Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value) override;

private:
    VKCommandQueue& m_command_queue;
    VKDevice& m_device;
    vk::UniqueSurfaceKHR m_surface;
    vk::Format m_swapchain_color_format = vk::Format::eUndefined;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<std::shared_ptr<Resource>> m_back_buffers;
    uint32_t m_frame_index = 0;
    vk::UniqueSemaphore m_image_available_semaphore;
    vk::UniqueSemaphore m_rendering_finished_semaphore;
    std::shared_ptr<CommandList> m_command_list;
    std::shared_ptr<Fence> m_fence;
};
