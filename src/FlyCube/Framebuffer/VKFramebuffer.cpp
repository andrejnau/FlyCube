#include "Framebuffer/VKFramebuffer.h"

#include "Device/VKDevice.h"
#include "Pipeline/VKGraphicsPipeline.h"
#include "View/VKView.h"

VKFramebuffer::VKFramebuffer(VKDevice& device, const FramebufferDesc& desc)
    : FramebufferBase(desc)
    , m_extent(desc.width, desc.height)
{
    vk::FramebufferCreateInfo framebuffer_info = {};
    std::vector<vk::ImageView> attachment_views;
    framebuffer_info.layers = 1;
    auto add_view = [&](const std::shared_ptr<View>& view) {
        if (!view) {
            return;
        }
        decltype(auto) vk_view = view->As<VKView>();
        decltype(auto) resource = vk_view.GetResource();
        if (!resource) {
            return;
        }
        attachment_views.emplace_back(vk_view.GetImageView());

        decltype(auto) vk_resource = resource->As<VKResource>();
        framebuffer_info.layers = std::max(framebuffer_info.layers, vk_resource.image.array_layers);
    };
    for (auto& rtv : desc.colors) {
        add_view(rtv);
    }
    add_view(desc.depth_stencil);
    add_view(desc.shading_rate_image);

    framebuffer_info.width = m_extent.width;
    framebuffer_info.height = m_extent.height;
    framebuffer_info.renderPass = desc.render_pass->As<VKRenderPass>().GetRenderPass();
    framebuffer_info.attachmentCount = attachment_views.size();
    framebuffer_info.pAttachments = attachment_views.data();
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
