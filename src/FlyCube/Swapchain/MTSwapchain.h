#pragma once
#include "Swapchain.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <memory>
#include <vector>

class MTDevice;

class MTSwapchain : public Swapchain {
public:
    MTSwapchain(MTDevice& device,
                const NativeSurface& surface,
                uint32_t width,
                uint32_t height,
                uint32_t frame_count,
                bool vsync);
    gli::format GetFormat() const override;
    std::shared_ptr<Resource> GetBackBuffer(uint32_t buffer) override;
    uint32_t NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value) override;
    void Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value) override;

private:
    MTDevice& device_;
    std::vector<std::shared_ptr<Resource>> back_buffers_;
    uint32_t frame_index_ = 0;
    uint32_t frame_count_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    CAMetalLayer* layer_ = nullptr;
};
