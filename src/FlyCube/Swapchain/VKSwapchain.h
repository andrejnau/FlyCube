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
                const NativeSurface& surface,
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
    VKCommandQueue& command_queue_;
    VKDevice& device_;
    vk::UniqueSurfaceKHR surface_;
    vk::Format swapchain_color_format_ = vk::Format::eUndefined;
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<std::shared_ptr<Resource>> back_buffers_;
    uint32_t frame_index_ = 0;
    std::shared_ptr<CommandList> command_list_;
    uint64_t fence_value_ = 0;
    std::shared_ptr<Fence> swapchain_fence_;
    uint32_t image_available_fence_index_ = 0;
    std::vector<uint64_t> image_available_fence_values_;
    std::vector<vk::UniqueSemaphore> image_available_semaphores_;
    std::vector<vk::UniqueSemaphore> rendering_finished_semaphores_;
};
