#pragma once
#include "RenderPass/RenderPass.h"

class RenderPassBase : public RenderPass {
public:
    RenderPassBase(const RenderPassDesc& desc);
    const RenderPassDesc& GetDesc() const override;

private:
    RenderPassDesc m_desc;
};
