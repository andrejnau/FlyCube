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
        color_attachment_references.emplace_back();
        add_attachment(color_attachment_references.back(), rtv.format, vk::ImageLayout::eColorAttachmentOptimal);
    }

    vk::SubpassDescription sub_pass = {};
    sub_pass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    sub_pass.colorAttachmentCount = color_attachment_references.size();
    sub_pass.pColorAttachments = color_attachment_references.data();

    vk::AttachmentReference depth_attachment_references;
    if (dsv.format != gli::FORMAT_UNDEFINED)
    {
        add_attachment(depth_attachment_references, dsv.format, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        sub_pass.pDepthStencilAttachment = &depth_attachment_references;
    }

    vk::AccessFlags access_mask =
        vk::AccessFlagBits::eIndirectCommandRead |
        vk::AccessFlagBits::eIndexRead |
        vk::AccessFlagBits::eVertexAttributeRead |
        vk::AccessFlagBits::eUniformRead |
        vk::AccessFlagBits::eInputAttachmentRead |
        vk::AccessFlagBits::eShaderRead |
        vk::AccessFlagBits::eShaderWrite |
        vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentRead |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite |
        vk::AccessFlagBits::eTransferRead |
        vk::AccessFlagBits::eTransferWrite |
        vk::AccessFlagBits::eHostRead |
        vk::AccessFlagBits::eHostWrite |
        vk::AccessFlagBits::eMemoryRead |
        vk::AccessFlagBits::eMemoryWrite;

    vk::SubpassDependency self_dependencie = {};
    self_dependencie.srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    self_dependencie.dstStageMask = vk::PipelineStageFlagBits::eAllCommands;
    self_dependencie.srcAccessMask = access_mask;
    self_dependencie.dstAccessMask = access_mask;
    self_dependencie.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo render_pass_info = {};
    render_pass_info.attachmentCount = attachment_descriptions.size();
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &sub_pass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &self_dependencie;

    m_render_pass = device.GetDevice().createRenderPassUnique(render_pass_info);
}

vk::RenderPass VKRenderPass::GetRenderPass() const
{
    return m_render_pass.get();
}
