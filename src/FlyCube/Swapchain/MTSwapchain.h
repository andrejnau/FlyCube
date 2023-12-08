#pragma once
#include "Swapchain.h"

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <memory>
#include <vector>

class MTDevice;

class MTSwapchain : public Swapchain {
public:
    MTSwapchain(MTDevice& device,
                WindowHandle window,
                uint32_t width,
                uint32_t height,
                uint32_t frame_count,
                bool vsync);
    gli::format GetFormat() const override;
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value) override;
    void Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value) override;

private:
    MTDevice& m_device;
    std::vector<std::shared_ptr<Resource>> m_back_buffers;
    uint32_t m_frame_index = 0;
    uint32_t m_frame_count = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    CAMetalLayer* m_layer = nullptr;
};
