#pragma once
#include "RenderPass/RenderPass.h"

class MTRenderPass : public RenderPass {
public:
    MTRenderPass(const RenderPassDesc& desc);
    const RenderPassDesc& GetDesc() const override;

private:
    RenderPassDesc m_desc;
};
