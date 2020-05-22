#pragma once
#include "Framebuffer/FramebufferBase.h"
#include <Utilities/Vulkan.h>

class VKDevice;
class VKGraphicsPipeline;

class VKFramebuffer : public FramebufferBase
{
public:
    VKFramebuffer(VKDevice& device, const std::shared_ptr<VKGraphicsPipeline>& pipeline, const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv);
    vk::Framebuffer GetFramebuffer() const;
    vk::RenderPass GetRenderPass() const;
    vk::Extent2D GetExtent() const;

private:
    vk::UniqueFramebuffer m_framebuffer;
    vk::RenderPass m_render_pass;
    vk::Extent2D m_extent;
};
