#pragma once
#include "Framebuffer/FramebufferBase.h"

#include <vulkan/vulkan.hpp>

class VKDevice;

class VKFramebuffer : public FramebufferBase {
public:
    VKFramebuffer(VKDevice& device, const FramebufferDesc& desc, vk::RenderPass render_pass);

    const std::vector<vk::ImageView>& GetAttachments() const;
    vk::UniqueFramebuffer TakeFramebuffer();

private:
    vk::UniqueFramebuffer m_framebuffer;
    std::vector<vk::ImageView> m_attachments;
};
