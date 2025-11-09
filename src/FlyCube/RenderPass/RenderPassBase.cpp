#include "RenderPass/RenderPassBase.h"

RenderPassBase::RenderPassBase(const RenderPassDesc& desc)
    : m_desc(desc)
{
}

const RenderPassDesc& RenderPassBase::GetDesc() const
{
    return m_desc;
}
