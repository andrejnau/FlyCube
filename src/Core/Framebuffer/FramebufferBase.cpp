#include "Framebuffer/FramebufferBase.h"

FramebufferBase::FramebufferBase(const FramebufferDesc& desc)
    : m_desc(desc)
{
}

const FramebufferDesc& FramebufferBase::GetDesc() const
{
    return m_desc;
}
