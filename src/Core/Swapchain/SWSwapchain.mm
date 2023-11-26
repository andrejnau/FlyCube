#include "Swapchain/SWSwapchain.h"
#include <Device/SWDevice.h>
#include <Resource/SWResource.h>
#include <Instance/SWInstance.h>

SWSwapchain::SWSwapchain(SWDevice& device, WindowHandle window, uint32_t width, uint32_t height, uint32_t frame_count, bool vsync)
    : m_device(device)
    , m_frame_count(frame_count)
    , m_width(width)
    , m_height(height)
    , m_window(window)
{
    m_back_buffers.resize(m_frame_count);
}

gli::format SWSwapchain::GetFormat() const
{
    return gli::FORMAT_BGRA8_UNORM_PACK8;
}

std::shared_ptr<Resource> SWSwapchain::GetBackBuffer(uint32_t buffer)
{
    return m_back_buffers[buffer];
}

uint32_t SWSwapchain::NextImage(const std::shared_ptr<Fence>& fence, uint64_t signal_value)
{
    return m_frame_index++ % m_frame_count;
}

void SWSwapchain::Present(const std::shared_ptr<Fence>& fence, uint64_t wait_value)
{
}
