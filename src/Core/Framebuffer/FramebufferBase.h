#pragma once
#include "Framebuffer/Framebuffer.h"
#include <View/View.h>
#include <vector>

class FramebufferBase
    : public Framebuffer
{
public:
    FramebufferBase(const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv);
    const std::vector<std::shared_ptr<View>>& GetRtvs() const;
    const std::shared_ptr<View> GetDsv() const;

private:
    std::vector<std::shared_ptr<View>> m_rtvs;
    std::shared_ptr<View> m_dsv;
};
