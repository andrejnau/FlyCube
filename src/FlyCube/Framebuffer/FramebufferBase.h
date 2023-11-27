#pragma once
#include "Framebuffer/Framebuffer.h"
#include "View/View.h"

#include <vector>

class FramebufferBase : public Framebuffer {
public:
    FramebufferBase(const FramebufferDesc& desc);
    const FramebufferDesc& GetDesc() const;

    std::shared_ptr<Resource>& GetDummyAttachment();

private:
    FramebufferDesc m_desc;
    std::shared_ptr<Resource> m_dummy_attachment;
};
