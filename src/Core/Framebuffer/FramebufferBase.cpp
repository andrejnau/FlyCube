#include "Framebuffer/FramebufferBase.h"

FramebufferBase::FramebufferBase(const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv)
    : m_rtvs(rtvs)
    , m_dsv(dsv)
{
}

const std::vector<std::shared_ptr<View>>& FramebufferBase::GetRtvs() const
{
    return m_rtvs;
}

const std::shared_ptr<View> FramebufferBase::GetDsv() const
{
    return m_dsv;
}
