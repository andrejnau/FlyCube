#include "Framebuffer/FramebufferBase.h"

FramebufferBase::FramebufferBase(const FramebufferDesc& desc)
    : m_desc(desc)
{
}

const FramebufferDesc& FramebufferBase::GetDesc() const
{
    return m_desc;
}

std::shared_ptr<Resource>& FramebufferBase::GetDummyAttachment()
{
    return m_dummy_attachment;
}
