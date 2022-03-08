#pragma once
#include "RenderPass/DXRenderPass.h"

DXRenderPass::DXRenderPass(const RenderPassDesc& desc)
    : m_desc(desc)
{
}

const RenderPassDesc& DXRenderPass::GetDesc() const
{
    return m_desc;
}
