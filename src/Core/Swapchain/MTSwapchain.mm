#include "Swapchain/MTSwapchain.h"
#include <Device/MTDevice.h>
#include <Resource/MTResource.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

id<MTLTexture> CrateTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
    MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];

    // Indicate that each pixel has a blue, green, red, and alpha channel, where each channel is
    // an 8-bit unsigned normalized value (i.e. 0 maps to 0.0 and 255 maps to 1.0)
    textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;

    // Set the pixel dimensions of the texture
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;

    // Create the texture from the device by using the descriptor
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    return texture;
}

MTSwapchain::MTSwapchain(MTDevice& device, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync)
    : m_device(device)
    , m_frame_count(frame_count)
    , m_width(width)
    , m_height(height)
{
    NSWindow* nswin = glfwGetCocoaWindow(window);
    m_layer = [CAMetalLayer layer];
    m_layer.drawableSize = CGSizeMake(width, height);
    m_layer.device = device.GetDevice();
    m_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_layer.maximumDrawableCount = frame_count;
    m_layer.displaySyncEnabled = vsync;
    m_layer.framebufferOnly = NO;
    nswin.contentView.layer = m_layer;
    nswin.contentView.wantsLayer = YES;

    for (size_t i = 0; i < frame_count; ++i)
    {
        std::shared_ptr<MTResource> res = std::make_shared<MTResource>(m_device);
        res->texture = CrateTexture(device.GetDevice(), width, height);
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
    return m_frame_index++ % m_frame_count;
}

void MTSwapchain::Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value)
{
    id<CAMetalDrawable> drawable = [m_layer nextDrawable];

    auto back_buffer = m_back_buffers[m_frame_index % m_frame_count];
    auto resource = back_buffer->As<MTResource>();
    auto queue = m_device.GetMTCommandQueue();

    auto command_buffer = [queue commandBuffer];
    id<MTLBlitCommandEncoder> blitEncoder = [command_buffer blitCommandEncoder];   
    [blitEncoder copyFromTexture:resource.texture
                 sourceSlice:0
                 sourceLevel:0
                 sourceOrigin:{0, 0, 0}
                 sourceSize:{m_width, m_height, 1}
                 toTexture:drawable.texture
                 destinationSlice:0
                 destinationLevel:0
                 destinationOrigin:{0, 0, 0}];
    [blitEncoder endEncoding];
    [command_buffer presentDrawable:drawable];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
}
