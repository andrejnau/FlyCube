#include "Framebuffer/VKFramebuffer.h"
#include <Device/VKDevice.h>
#include <View/VKView.h>
#include <Pipeline/VKGraphicsPipeline.h>

VKFramebuffer::VKFramebuffer(VKDevice& device, const std::shared_ptr<RenderPass>& render_pass, uint32_t width, uint32_t height,
                             const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv)
    : FramebufferBase(rtvs, dsv)
    , m_extent(width, height)
{
    vk::FramebufferCreateInfo framebuffer_info = {};
    std::vector<vk::ImageView> attachment_views;
    framebuffer_info.layers = 1;
    auto add_view = [&](const std::shared_ptr<View>& view)
    {
        if (!view)
            return;
        decltype(auto) vk_view = view->As<VKView>();
        decltype(auto) resource = vk_view.GetResource();
        if (!resource)
            return;
        attachment_views.emplace_back(vk_view.GetRtv());

        decltype(auto) vk_resource = resource->As<VKResource>();
        if (!m_extent.width || !m_extent.height)
            m_extent = vk::Extent2D(vk_resource.image.size.width, vk_resource.image.size.height);
        framebuffer_info.layers = std::max<uint32_t>(framebuffer_info.layers, vk_resource.image.array_layers);
    };
    for (auto& rtv : rtvs)
    {
        add_view(rtv);
    }
    add_view(dsv);

    framebuffer_info.width = m_extent.width;
    framebuffer_info.height = m_extent.height;
    framebuffer_info.renderPass = render_pass->As<VKRenderPass>().GetRenderPass();
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
