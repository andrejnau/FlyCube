#pragma once

#include <FrameBuffer/FrameBuffer.h>
#include <Geometry/Geometry.h>
#include <SwapChain/SwapChain.h>
#include <Program/Program.h>
#include <Texture/Texture.h>
#include <memory>

enum class ApiType
{
    kGL,
    kDX11
};

class Context
{
public:
    using Ptr = std::shared_ptr<Context>;

    virtual FrameBuffer::Ptr CreateFrameBuffer() = 0;
    virtual Geometry::Ptr CreateGeometry() = 0;
    virtual Program::Ptr CreateProgram() = 0;
    virtual SwapChain::Ptr CreateSwapChain() = 0;
    virtual Texture::Ptr CreateTexture() = 0;
};

Context::Ptr CreareContext(ApiType api_type, int width, int height);
