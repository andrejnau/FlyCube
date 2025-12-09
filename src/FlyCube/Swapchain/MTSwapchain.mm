#include "Swapchain/MTSwapchain.h"

#include "Device/MTDevice.h"
#include "Fence/MTFence.h"
#include "Instance/MTInstance.h"
#include "Resource/MTTexture.h"

MTSwapchain::MTSwapchain(MTDevice& device,
                         const NativeSurface& surface,
                         uint32_t width,
                         uint32_t height,
                         uint32_t frame_count,
                         bool vsync)
    : device_(device)
    , frame_index_(frame_count - 1)
    , frame_count_(frame_count)
    , width_(width)
    , height_(height)
{
    const auto& metal_surface = std::get<MetalSurface>(surface);
    layer_ = (__bridge CAMetalLayer*)metal_surface.ca_metal_layer;
    layer_.drawableSize = CGSizeMake(width, height);
    layer_.device = device.GetDevice();
    layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer_.maximumDrawableCount = frame_count;
#if TARGET_OS_OSX
    layer_.displaySyncEnabled = vsync;
#endif
    layer_.framebufferOnly = NO;

    for (size_t i = 0; i < frame_count; ++i) {
        TextureDesc texture_desc = {
            .type = TextureType::k2D,
            .format = GetFormat(),
            .width = width,
            .height = height,
            .depth_or_array_layers = 1,
            .mip_levels = 1,
            .sample_count = 1,
            .usage = BindFlag::kRenderTarget | BindFlag::kUnorderedAccess,
        };
        std::shared_ptr<MTTexture> back_buffer = MTTexture::CreateSwapchainTexture(device_, texture_desc);
        back_buffer->CommitMemory(MemoryType::kDefault);
        back_buffers_.emplace_back(std::move(back_buffer));
    }
}

gli::format MTSwapchain::GetFormat() const
{
    return gli::FORMAT_BGRA8_UNORM_PACK8;
}

std::shared_ptr<Resource> MTSwapchain::GetBackBuffer(uint32_t buffer)
{
    return back_buffers_[buffer];
}

uint32_t MTSwapchain::NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    auto queue = device_.GetMTCommandQueue();
    [queue signalEvent:mt_fence.GetSharedEvent() value:signal_value];
    return frame_index_ = (frame_index_ + 1) % frame_count_;
}

void MTSwapchain::Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    decltype(auto) resource = back_buffers_[frame_index_]->As<MTResource>();
    auto queue = device_.GetMTCommandQueue();
    id<CAMetalDrawable> drawable = [layer_ nextDrawable];

    auto command_buffer = [device_.GetDevice() newCommandBuffer];
    auto allocator = [device_.GetDevice() newCommandAllocator];
    [command_buffer beginCommandBufferWithAllocator:allocator];

    auto residency_set = device_.CreateResidencySet();
    [residency_set addAllocation:resource.GetTexture()];
    [residency_set addAllocation:drawable.texture];
    [residency_set commit];
    [command_buffer useResidencySet:residency_set];

    auto compute_encoder = [command_buffer computeCommandEncoder];
    [compute_encoder copyFromTexture:resource.GetTexture()
                         sourceSlice:0
                         sourceLevel:0
                        sourceOrigin:{ 0, 0, 0 }
                          sourceSize:{ width_, height_, 1 }
                           toTexture:drawable.texture
                    destinationSlice:0
                    destinationLevel:0
                   destinationOrigin:{ 0, 0, 0 }];
    [compute_encoder endEncoding];

    [command_buffer endCommandBuffer];

    [queue waitForEvent:mt_fence.GetSharedEvent() value:wait_value];
    [queue waitForDrawable:drawable];
    [queue commit:&command_buffer count:1];
    [queue signalDrawable:drawable];
    [drawable present];
}
