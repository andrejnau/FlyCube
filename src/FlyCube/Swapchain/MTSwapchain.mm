#include "Swapchain/MTSwapchain.h"

#include "Device/MTDevice.h"
#include "Fence/MTFence.h"
#include "Instance/MTInstance.h"
#include "Resource/MTResource.h"

MTSwapchain::MTSwapchain(MTDevice& device,
                         const NativeSurface& surface,
                         uint32_t width,
                         uint32_t height,
                         uint32_t frame_count,
                         bool vsync)
    : m_device(device)
    , m_frame_index(frame_count - 1)
    , m_frame_count(frame_count)
    , m_width(width)
    , m_height(height)
{
    const auto& metal_surface = std::get<MetalSurface>(surface);
    m_layer = (__bridge CAMetalLayer*)metal_surface.ca_metal_layer;
    m_layer.drawableSize = CGSizeMake(width, height);
    m_layer.device = device.GetDevice();
    m_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_layer.maximumDrawableCount = frame_count;
#if TARGET_OS_OSX
    m_layer.displaySyncEnabled = vsync;
#endif
    m_layer.framebufferOnly = NO;

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
        std::shared_ptr<MTResource> back_buffer = MTResource::CreateSwapchainTexture(m_device, texture_desc);
        back_buffer->CommitMemory(MemoryType::kDefault);
        m_back_buffers.emplace_back(std::move(back_buffer));
    }
}

gli::format MTSwapchain::GetFormat() const
{
    return gli::FORMAT_BGRA8_UNORM_PACK8;
}

std::shared_ptr<Resource> MTSwapchain::GetBackBuffer(uint32_t buffer)
{
    return m_back_buffers[buffer];
}

uint32_t MTSwapchain::NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    auto queue = m_device.GetMTCommandQueue();
    [queue signalEvent:mt_fence.GetSharedEvent() value:signal_value];
    return m_frame_index = (m_frame_index + 1) % m_frame_count;
}

void MTSwapchain::Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value)
{
    decltype(auto) mt_fence = fence->As<MTFence>();
    auto back_buffer = m_back_buffers[m_frame_index];
    auto resource = back_buffer->As<MTResource>();
    auto queue = m_device.GetMTCommandQueue();
    id<CAMetalDrawable> drawable = [m_layer nextDrawable];

    auto command_buffer = [m_device.GetDevice() newCommandBuffer];
    auto allocator = [m_device.GetDevice() newCommandAllocator];
    [command_buffer beginCommandBufferWithAllocator:allocator];

    auto residency_set = m_device.CreateResidencySet();
    [residency_set addAllocation:resource.GetTexture()];
    [residency_set addAllocation:drawable.texture];
    [residency_set commit];
    [command_buffer useResidencySet:residency_set];

    auto compute_encoder = [command_buffer computeCommandEncoder];
    [compute_encoder copyFromTexture:resource.GetTexture()
                         sourceSlice:0
                         sourceLevel:0
                        sourceOrigin:{ 0, 0, 0 }
                          sourceSize:{ m_width, m_height, 1 }
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
