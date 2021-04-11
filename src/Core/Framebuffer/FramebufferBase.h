#pragma once
#include "Framebuffer/Framebuffer.h"
#include <View/View.h>
#include <vector>

class FramebufferBase
    : public Framebuffer
{
public:
    FramebufferBase(const FramebufferDesc& desc);
    const FramebufferDesc& GetDesc() const;

private:
    FramebufferDesc m_desc;
};
