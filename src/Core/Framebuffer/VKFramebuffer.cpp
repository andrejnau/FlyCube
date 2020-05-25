#include "Framebuffer/VKFramebuffer.h"
#include <Device/VKDevice.h>
#include <View/VKView.h>
#include <Pipeline/VKGraphicsPipeline.h>

VKFramebuffer::VKFramebuffer(VKDevice& device, const std::shared_ptr<VKGraphicsPipeline>& pipeline, const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv)
    : FramebufferBase(rtvs, dsv)
{
    vk::FramebufferCreateInfo framebuffer_info = {};
    std::vector<vk::ImageView> attachment_views;
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
        {
            m_extent = vk::Extent2D(vk_resource.image.size.width, vk_resource.image.size.height);
            framebuffer_info.width = m_extent.width;
            framebuffer_info.height = m_extent.height;
            framebuffer_info.layers = vk_resource.image.array_layers;
        }
    };
    for (auto& rtv : rtvs)
    {
        add_view(rtv);
    }
    add_view(dsv);

    m_render_pass = pipeline->GetRenderPass();

    framebuffer_info.renderPass = m_render_pass;
    framebuffer_info.attachmentCount = attachment_views.size();
    framebuffer_info.pAttachments = attachment_views.data();

    m_framebuffer = device.GetDevice().createFramebufferUnique(framebuffer_info);
}

vk::Framebuffer VKFramebuffer::GetFramebuffer() const
{
    return m_framebuffer.get();
}

vk::RenderPass VKFramebuffer::GetRenderPass() const
{
    return m_render_pass;
}

vk::Extent2D VKFramebuffer::GetExtent() const
{
    return m_extent;
}
