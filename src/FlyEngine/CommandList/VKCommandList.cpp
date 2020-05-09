#include "CommandList/VKCommandList.h"
#include <Device/VKDevice.h>
#include <Resource/VKResource.h>

VKCommandList::VKCommandList(VKDevice& device)
{
    vk::CommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.commandPool = device.GetCmdPool();
    cmd_buf_alloc_info.commandBufferCount = 1;
    cmd_buf_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmd_bufs = device.GetDevice().allocateCommandBuffersUnique(cmd_buf_alloc_info);
    m_cmd_buf = std::move(cmd_bufs.front());
}

void VKCommandList::Open()
{
    vk::CommandBufferBeginInfo begin_info = {};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    m_cmd_buf->begin(begin_info);
}

void VKCommandList::Close()
{
    m_cmd_buf->end();
}

void VKCommandList::Clear(Resource::Ptr iresource, const std::array<float, 4>& color)
{
    VKResource& resource = (VKResource&)*iresource;
    vk::ClearColorValue clear_color = {};
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];

    vk::ImageSubresourceRange ImageSubresourceRange;
    ImageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    ImageSubresourceRange.baseMipLevel = 0;
    ImageSubresourceRange.levelCount = 1;
    ImageSubresourceRange.baseArrayLayer = 0;
    ImageSubresourceRange.layerCount = 1;
    m_cmd_buf->clearColorImage(resource.image.res.get(), vk::ImageLayout::eTransferDstOptimal, clear_color, ImageSubresourceRange);
}

vk::CommandBuffer VKCommandList::GetCommandList()
{
    return m_cmd_buf.get();
}
