#include "CommandList/VKCommandList.h"
#include <Device/VKDevice.h>
#include <Resource/VKResource.h>
#include <View/VKView.h>

VKCommandList::VKCommandList(VKDevice& device)
    : m_device(device)
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

void VKCommandList::Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color)
{
    if (!view)
        return;
    VKView& vk_view = static_cast<VKView&>(*view);

    std::shared_ptr<Resource> resource = vk_view.GetResource();
    if (!resource)
        return;
    VKResource& vk_resource = static_cast<VKResource&>(*resource);

    vk::ClearColorValue clear_color = {};
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];

    const vk::ImageSubresourceRange& ImageSubresourceRange = vk_view.GeViewInfo().subresourceRange;
    m_cmd_buf->clearColorImage(vk_resource.image.res.get(), vk_resource.image.layout[ImageSubresourceRange], clear_color, ImageSubresourceRange);
}

void VKCommandList::ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    return ResourceBarrier(resource, {}, state);
}

void VKCommandList::ResourceBarrier(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc, ResourceState state)
{
    VKResource& vk_resource = static_cast<VKResource&>(*resource);
    VKResource::Image& image = vk_resource.image;

    vk::ImageLayout new_layout = vk::ImageLayout::eUndefined;
    switch (state)
    {
    case ResourceState::kCommon:
    case ResourceState::kClear:
        new_layout = vk::ImageLayout::eGeneral;
        break;
    case ResourceState::kPresent:
        new_layout = vk::ImageLayout::ePresentSrcKHR;
        break;
    case ResourceState::kRenderTarget:
    case ResourceState::kUnorderedAccess:
        new_layout = vk::ImageLayout::eColorAttachmentOptimal;
        break;
    }

    vk::ImageSubresourceRange range = {};
    range.aspectMask = m_device.GetAspectFlags(image.format);
    range.baseMipLevel = view_desc.level;
    if (new_layout == vk::ImageLayout::eColorAttachmentOptimal)
        range.levelCount = 1;
    else if (view_desc.count == -1)
        range.levelCount = image.level_count - view_desc.level;
    else
        range.levelCount = view_desc.count;
    range.baseArrayLayer = 0;
    range.layerCount = image.array_layers;

    std::vector<vk::ImageMemoryBarrier> image_memory_barriers;

    for (uint32_t i = 0; i < range.levelCount; ++i)
    {
        for (uint32_t j = 0; j < range.layerCount; ++j)
        {
            vk::ImageSubresourceRange barrier_range = range;
            barrier_range.baseMipLevel = range.baseMipLevel + i;
            barrier_range.levelCount = 1;
            barrier_range.baseArrayLayer = range.baseArrayLayer + j;
            barrier_range.layerCount = 1;

            if (image.layout[barrier_range] == new_layout)
                continue;

            image_memory_barriers.emplace_back();
            vk::ImageMemoryBarrier& imageMemoryBarrier = image_memory_barriers.back();
            imageMemoryBarrier.oldLayout = image.layout[barrier_range];

            imageMemoryBarrier.newLayout = new_layout;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = image.res.get();
            imageMemoryBarrier.subresourceRange = barrier_range;

            // Source layouts (old)
            // Source access mask controls actions that have to be finished on the old layout
            // before it will be transitioned to the new layout
            switch (image.layout[barrier_range])
            {
            case vk::ImageLayout::eUndefined:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = {};
                break;
            case vk::ImageLayout::ePreinitialized:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
                break;
            case vk::ImageLayout::eColorAttachmentOptimal:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthAttachmentOptimal:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                // Image is a transfer source 
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            // Target layouts (new)
            // Destination access mask controls the dependency for the new image layout
            switch (new_layout)
            {
            case vk::ImageLayout::eTransferDstOptimal:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;

            case vk::ImageLayout::eTransferSrcOptimal:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
                break;

            case vk::ImageLayout::eColorAttachmentOptimal:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;

            case vk::ImageLayout::eDepthAttachmentOptimal:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (!imageMemoryBarrier.srcAccessMask)
                {
                    imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
                }
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            image.layout[barrier_range] = new_layout;
        }
    }

    m_cmd_buf->pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        0, nullptr,
        0, nullptr,
        image_memory_barriers.size(), image_memory_barriers.data());
}

vk::CommandBuffer VKCommandList::GetCommandList()
{
    return m_cmd_buf.get();
}
