#pragma once
#include "Framebuffer/FramebufferBase.h"

#include <vulkan/vulkan.hpp>

class VKDevice;
class VKGraphicsPipeline;

class VKFramebuffer : public FramebufferBase {
public:
    VKFramebuffer(VKDevice& device, const FramebufferDesc& desc);

    vk::Framebuffer GetFramebuffer() const;
    vk::Extent2D GetExtent() const;

private:
    vk::UniqueFramebuffer m_framebuffer;
    vk::Extent2D m_extent;
};
