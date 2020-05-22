#pragma once
#include "RenderPass/RenderPass.h"
#include <Utilities/Vulkan.h>

class VKDevice;

class VKRenderPass : public RenderPass
{
public:
    VKRenderPass(VKDevice& device, const std::vector<RenderTargetDesc>& rtvs, const DepthStencilTargetDesc& dsv);
    vk::RenderPass GetRenderPass() const;

private:
    vk::UniqueRenderPass m_render_pass;
};
