#include "RenderPass/MTRenderPass.h"

MTRenderPass::MTRenderPass(const RenderPassDesc& desc)
    : m_desc(desc)
{
}

const RenderPassDesc& MTRenderPass::GetDesc() const
{
    return m_desc;
}
