#pragma once
#include "Framebuffer/FramebufferBase.h"
#include <vulkan/vulkan.hpp>

class VKDevice;
class VKGraphicsPipeline;

class VKFramebuffer : public FramebufferBase
{
public:
    VKFramebuffer(VKDevice& device, const std::shared_ptr<RenderPass>& render_pass, uint32_t width, uint32_t height,
                  const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv);
    vk::Framebuffer GetFramebuffer() const;
    vk::Extent2D GetExtent() const;

private:
    vk::UniqueFramebuffer m_framebuffer;
    vk::Extent2D m_extent;
};
