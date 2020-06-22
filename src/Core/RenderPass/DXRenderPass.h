#pragma once
#include "RenderPass/RenderPass.h"

class DXDevice;

class DXRenderPass : public RenderPass
{
public:
    DXRenderPass(DXDevice& device, const RenderPassDesc& desc);
    const RenderPassDesc& GetDesc() const override;

private:
    RenderPassDesc m_desc;
};
