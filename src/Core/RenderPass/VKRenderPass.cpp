#include "RenderPass/VKRenderPass.h"
#include <Device/VKDevice.h>
#include <View/VKView.h>

VKRenderPass::VKRenderPass(VKDevice& device, const std::vector<RenderTargetDesc>& rtvs, const DepthStencilTargetDesc& dsv)
{
    std::vector<vk::AttachmentDescription> attachment_descriptions;
    auto add_attachment = [&](vk::AttachmentReference& reference, gli::format format, vk::ImageLayout layout)
    {
        attachment_descriptions.emplace_back();
        vk::AttachmentDescription& description = attachment_descriptions.back();
        description.format = static_cast<vk::Format>(format);
        description.samples = static_cast<vk::SampleCountFlagBits>(1);
        description.loadOp = vk::AttachmentLoadOp::eLoad;
        description.storeOp = vk::AttachmentStoreOp::eStore;
        description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        description.initialLayout = layout;
        description.finalLayout = layout;

        reference.attachment = attachment_descriptions.size() - 1;
        reference.layout = layout;
    };

    std::vector<vk::AttachmentReference> color_attachment_references;
    for (auto& rtv : rtvs)
    {
        if (rtv.format == gli::FORMAT_UNDEFINED)
            continue;
        if (rtv.slot >= color_attachment_references.size())
            color_attachment_references.resize(rtv.slot + 1, { VK_ATTACHMENT_UNUSED });
        add_attachment(color_attachment_references[rtv.slot], rtv.format, vk::ImageLayout::eColorAttachmentOptimal);
    }

    vk::SubpassDescription sub_pass = {};
    sub_pass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    sub_pass.colorAttachmentCount = color_attachment_references.size();
    sub_pass.pColorAttachments = color_attachment_references.data();

    vk::AttachmentReference depth_attachment_references = {};
    if (dsv.format != gli::FORMAT_UNDEFINED)
    {
        add_attachment(depth_attachment_references, dsv.format, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        sub_pass.pDepthStencilAttachment = &depth_attachment_references;
    }

    vk::RenderPassCreateInfo render_pass_info = {};
    render_pass_info.attachmentCount = attachment_descriptions.size();
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &sub_pass;

    m_render_pass = device.GetDevice().createRenderPassUnique(render_pass_info);
}

vk::RenderPass VKRenderPass::GetRenderPass() const
{
    return m_render_pass.get();
}
