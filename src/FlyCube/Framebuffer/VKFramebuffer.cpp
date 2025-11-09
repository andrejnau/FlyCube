#include "Framebuffer/VKFramebuffer.h"

#include "Device/VKDevice.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "View/VKView.h"

VKFramebuffer::VKFramebuffer(VKDevice& device, const FramebufferDesc& desc)
    : FramebufferBase(desc)
    , m_extent(desc.width, desc.height)
{
    vk::FramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.width = m_extent.width;
    framebuffer_info.height = m_extent.height;
    framebuffer_info.layers = std::numeric_limits<uint32_t>::max();

    std::deque<vk::Format> formats;
    std::vector<vk::FramebufferAttachmentImageInfo> attachment_image_infos = {};
    auto add_view = [&](const std::shared_ptr<View>& view) {
        if (!view) {
            return;
        }
        decltype(auto) vk_view = view->As<VKView>();
        decltype(auto) resource = vk_view.GetResource();
        if (!resource) {
            return;
        }
        decltype(auto) vk_resource = resource->As<VKResource>();

        formats.push_back(static_cast<vk::Format>(vk_resource.GetFormat()));

        vk::FramebufferAttachmentImageInfo frame_buffer_attachment_image_info = {};
        frame_buffer_attachment_image_info.flags = vk_resource.GetImageCreateFlags();
        frame_buffer_attachment_image_info.usage = vk_resource.GetImageUsageFlags();
        frame_buffer_attachment_image_info.width = vk_resource.GetWidth() >> vk_view.GetBaseMipLevel();
        frame_buffer_attachment_image_info.height = vk_resource.GetHeight() >> vk_view.GetBaseMipLevel();
        frame_buffer_attachment_image_info.layerCount = vk_view.GetLayerCount();
        frame_buffer_attachment_image_info.viewFormatCount = 1;
        frame_buffer_attachment_image_info.pViewFormats = &formats.back();
        attachment_image_infos.push_back(frame_buffer_attachment_image_info);

        m_attachments.emplace_back(vk_view.GetImageView());

        assert(frame_buffer_attachment_image_info.width >= framebuffer_info.width);
        assert(frame_buffer_attachment_image_info.height >= framebuffer_info.height);
        framebuffer_info.layers = std::min(framebuffer_info.layers, frame_buffer_attachment_image_info.layerCount);
    };
    for (auto& rtv : desc.colors) {
        add_view(rtv);
    }
    add_view(desc.depth_stencil);
    add_view(desc.shading_rate_image);

    if (framebuffer_info.layers == std::numeric_limits<uint32_t>::max()) {
        framebuffer_info.layers = 1;
    }

    framebuffer_info.renderPass = desc.render_pass->As<VKRenderPass>().GetRenderPass();
    framebuffer_info.flags = vk::FramebufferCreateFlagBits::eImageless;
    framebuffer_info.attachmentCount = attachment_image_infos.size();
    vk::FramebufferAttachmentsCreateInfo framebuffer_attachments_info = {};
    framebuffer_attachments_info.attachmentImageInfoCount = attachment_image_infos.size();
    framebuffer_attachments_info.pAttachmentImageInfos = attachment_image_infos.data();
    framebuffer_info.pNext = &framebuffer_attachments_info;
    m_framebuffer = device.GetDevice().createFramebufferUnique(framebuffer_info);
}

vk::Framebuffer VKFramebuffer::GetFramebuffer() const
{
    return m_framebuffer.get();
}

vk::Extent2D VKFramebuffer::GetExtent() const
{
    return m_extent;
}

const std::vector<vk::ImageView>& VKFramebuffer::GetAttachments() const
{
    return m_attachments;
}
