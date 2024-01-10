#include "Swapchain/MTSwapchain.h"

#include "Device/MTDevice.h"
#include "Instance/MTInstance.h"
#include "Resource/MTResource.h"

namespace {

id<MTLTexture> CrateTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
    MTLTextureDescriptor* texture_descriptor = [[MTLTextureDescriptor alloc] init];
    texture_descriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
    texture_descriptor.width = width;
    texture_descriptor.height = height;
    texture_descriptor.usage = MTLTextureUsageRenderTarget;
    return [device newTextureWithDescriptor:texture_descriptor];
}

} // namespace

MTSwapchain::MTSwapchain(MTDevice& device,
                         WindowHandle window,
                         uint32_t width,
                         uint32_t height,
                         uint32_t frame_count,
                         bool vsync)
    : m_device(device)
    , m_frame_index(frame_count - 1)
    , m_frame_count(frame_count)
    , m_width(width)
    , m_height(height)
    , m_layer((__bridge CAMetalLayer*)window)
{
    m_layer.drawableSize = CGSizeMake(width, height);
    m_layer.device = device.GetDevice();
    m_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_layer.maximumDrawableCount = frame_count;
#if TARGET_OS_OSX
    m_layer.displaySyncEnabled = vsync;
#endif
    m_layer.framebufferOnly = NO;

    for (size_t i = 0; i < frame_count; ++i) {
        std::shared_ptr<MTResource> res = std::make_shared<MTResource>(m_device);
        res->texture.res = CrateTexture(device.GetDevice(), width, height);
        res->is_back_buffer = true;
        res->resource_type = ResourceType::kTexture;
        res->format = GetFormat();
        m_back_buffers.emplace_back(res);
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
    return m_frame_index = (m_frame_index + 1) % m_frame_count;
}

void MTSwapchain::Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value)
{
    auto back_buffer = m_back_buffers[m_frame_index];
    auto resource = back_buffer->As<MTResource>();
    auto queue = m_device.GetMTCommandQueue();

    auto command_buffer = [queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
    id<CAMetalDrawable> drawable = [m_layer nextDrawable];
    [blit_encoder copyFromTexture:resource.texture.res
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:{ 0, 0, 0 }
                       sourceSize:{ m_width, m_height, 1 }
                        toTexture:drawable.texture
                 destinationSlice:0
                 destinationLevel:0
                destinationOrigin:{ 0, 0, 0 }];
    [blit_encoder endEncoding];
    [command_buffer addScheduledHandler:^(id<MTLCommandBuffer>) {
        [drawable present];
    }];
    [command_buffer commit];
}
