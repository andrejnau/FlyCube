#pragma once
#include "Framebuffer/FramebufferBase.h"
#import <Metal/Metal.h>

class MTDevice;

class MTFramebuffer : public FramebufferBase
{
public:
    MTFramebuffer(MTDevice& device, const FramebufferDesc& desc);

private:
    MTLRenderPassDescriptor* m_render_pass_descriptor = nullptr;
};
