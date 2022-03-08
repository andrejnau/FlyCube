#include "Framebuffer/MTFramebuffer.h"
#include <Device/MTDevice.h>
#include <View/MTView.h>
#include <Resource/MTResource.h>

static MTLLoadAction Convert(RenderPassLoadOp op)
{
    switch (op)
    {
    case RenderPassLoadOp::kLoad:
        return MTLLoadActionLoad;
    case RenderPassLoadOp::kClear:
        return MTLLoadActionClear;
    case RenderPassLoadOp::kDontCare:
        return MTLLoadActionDontCare;
    }
    assert(false);
    return MTLLoadActionLoad;
}

static MTLStoreAction Convert(RenderPassStoreOp op)
{
    switch (op)
    {
    case RenderPassStoreOp::kStore:
        return MTLStoreActionStore;
    case RenderPassStoreOp::kDontCare:
        return MTLStoreActionDontCare;
    }
    assert(false);
    return MTLStoreActionStore;
}

MTFramebuffer::MTFramebuffer(MTDevice& device, const FramebufferDesc& desc)
    : FramebufferBase(desc)
{
    m_render_pass_descriptor = [MTLRenderPassDescriptor new];

    const RenderPassDesc& render_pass_desc = desc.render_pass->GetDesc();
    for (size_t i = 0; i < render_pass_desc.colors.size(); ++i)
    {
        decltype(auto) attachment = m_render_pass_descriptor.colorAttachments[i];
        decltype(auto) render_pass_color = render_pass_desc.colors[i];
        attachment.loadAction = Convert(render_pass_color.load_op);
        attachment.storeAction = Convert(render_pass_color.store_op);

        decltype(auto) mt_view = desc.colors[i]->As<MTView>();
        attachment.level = mt_view.GetBaseMipLevel();
        attachment.slice = mt_view.GetBaseArrayLayer();

        decltype(auto) resource = mt_view.GetResource();
        if (!resource)
            continue;
        decltype(auto) mt_resource = resource->As<MTResource>();
        attachment.texture = mt_resource.texture;
    }
}
