#pragma once
#include "RenderPass/RenderPass.h"

class DXRenderPass : public RenderPass {
public:
    DXRenderPass(const RenderPassDesc& desc);
    const RenderPassDesc& GetDesc() const override;

private:
    RenderPassDesc m_desc;
};
