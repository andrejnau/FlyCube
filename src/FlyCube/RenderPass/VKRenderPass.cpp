#include "RenderPass/VKRenderPass.h"

#include "Device/VKDevice.h"
#include "View/VKView.h"

namespace {

vk::AttachmentLoadOp Convert(RenderPassLoadOp op)
{
    switch (op) {
    case RenderPassLoadOp::kLoad:
        return vk::AttachmentLoadOp::eLoad;
    case RenderPassLoadOp::kClear:
        return vk::AttachmentLoadOp::eClear;
    case RenderPassLoadOp::kDontCare:
        return vk::AttachmentLoadOp::eDontCare;
    }
    assert(false);
    return vk::AttachmentLoadOp::eLoad;
}

vk::AttachmentStoreOp Convert(RenderPassStoreOp op)
{
    switch (op) {
    case RenderPassStoreOp::kStore:
        return vk::AttachmentStoreOp::eStore;
    case RenderPassStoreOp::kDontCare:
        return vk::AttachmentStoreOp::eDontCare;
    }
    assert(false);
    return vk::AttachmentStoreOp::eStore;
}

} // namespace

VKRenderPass::VKRenderPass(VKDevice& device, const RenderPassDesc& desc)
    : m_desc(desc)
{
    while (!m_desc.colors.empty() && m_desc.colors.back().format == gli::FORMAT_UNDEFINED) {
        m_desc.colors.pop_back();
    }

    std::vector<vk::AttachmentDescription2> attachment_descriptions;
    auto add_attachment = [&](vk::AttachmentReference2& reference, gli::format format, vk::ImageLayout layout,
                              RenderPassLoadOp load_op, RenderPassStoreOp store_op) {
        if (format == gli::FORMAT_UNDEFINED) {
            reference.attachment = VK_ATTACHMENT_UNUSED;
            return;
        }
        vk::AttachmentDescription2& description = attachment_descriptions.emplace_back();
        description.format = static_cast<vk::Format>(format);
        description.samples = static_cast<vk::SampleCountFlagBits>(m_desc.sample_count);
        description.loadOp = Convert(load_op);
        description.storeOp = Convert(store_op);
        description.initialLayout = layout;
        description.finalLayout = layout;

        reference.attachment = attachment_descriptions.size() - 1;
        reference.layout = layout;
    };

    vk::SubpassDescription2 sub_pass = {};
    sub_pass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

    std::vector<vk::AttachmentReference2> color_attachment_references;
    for (auto& rtv : m_desc.colors) {
        add_attachment(color_attachment_references.emplace_back(), rtv.format, vk::ImageLayout::eColorAttachmentOptimal,
                       rtv.load_op, rtv.store_op);
    }

    sub_pass.colorAttachmentCount = color_attachment_references.size();
    sub_pass.pColorAttachments = color_attachment_references.data();

    vk::AttachmentReference2 depth_attachment_reference = {};
    if (m_desc.depth_stencil.format != gli::FORMAT_UNDEFINED) {
        add_attachment(depth_attachment_reference, m_desc.depth_stencil.format,
                       vk::ImageLayout::eDepthStencilAttachmentOptimal, m_desc.depth_stencil.depth_load_op,
                       m_desc.depth_stencil.depth_store_op);
        if (depth_attachment_reference.attachment != VK_ATTACHMENT_UNUSED) {
            vk::AttachmentDescription2& description = attachment_descriptions[depth_attachment_reference.attachment];
            description.stencilLoadOp = Convert(m_desc.depth_stencil.stencil_load_op);
            description.stencilStoreOp = Convert(m_desc.depth_stencil.stencil_store_op);
        }
        sub_pass.pDepthStencilAttachment = &depth_attachment_reference;
    }

    if (m_desc.shading_rate_format != gli::FORMAT_UNDEFINED) {
        vk::AttachmentReference2 shading_rate_image_attachment_reference = {};
        add_attachment(shading_rate_image_attachment_reference, m_desc.shading_rate_format,
                       vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR, RenderPassLoadOp::kLoad,
                       RenderPassStoreOp::kStore);

        vk::FragmentShadingRateAttachmentInfoKHR fragment_shading_rate_attachment_info = {};
        fragment_shading_rate_attachment_info.pFragmentShadingRateAttachment = &shading_rate_image_attachment_reference;
        fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.width =
            device.GetShadingRateImageTileSize();
        fragment_shading_rate_attachment_info.shadingRateAttachmentTexelSize.height =
            device.GetShadingRateImageTileSize();
        sub_pass.pNext = &fragment_shading_rate_attachment_info;
    }

    vk::RenderPassCreateInfo2 render_pass_info = {};
    render_pass_info.attachmentCount = attachment_descriptions.size();
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &sub_pass;

    m_render_pass = device.GetDevice().createRenderPass2KHRUnique(render_pass_info);
}

const RenderPassDesc& VKRenderPass::GetDesc() const
{
    return m_desc;
}

vk::RenderPass VKRenderPass::GetRenderPass() const
{
    return m_render_pass.get();
}
