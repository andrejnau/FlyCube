#include "Framebuffer/VKFramebuffer.h"

#include "Device/VKDevice.h"
#include "View/VKView.h"

#include <deque>

VKFramebuffer::VKFramebuffer(VKDevice& device, const FramebufferDesc& desc, vk::RenderPass render_pass)
    : FramebufferBase(desc)
{
    vk::FramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.width = desc.width;
    framebuffer_info.height = desc.height;
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

    framebuffer_info.renderPass = render_pass;
    framebuffer_info.flags = vk::FramebufferCreateFlagBits::eImageless;
    framebuffer_info.attachmentCount = attachment_image_infos.size();
    vk::FramebufferAttachmentsCreateInfo framebuffer_attachments_info = {};
    framebuffer_attachments_info.attachmentImageInfoCount = attachment_image_infos.size();
    framebuffer_attachments_info.pAttachmentImageInfos = attachment_image_infos.data();
    framebuffer_info.pNext = &framebuffer_attachments_info;
    m_framebuffer = device.GetDevice().createFramebufferUnique(framebuffer_info);
}

const std::vector<vk::ImageView>& VKFramebuffer::GetAttachments() const
{
    return m_attachments;
}

vk::UniqueFramebuffer VKFramebuffer::TakeFramebuffer()
{
    return std::move(m_framebuffer);
}
