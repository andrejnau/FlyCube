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
    virtual Geometry::Ptr CreateGeometry(const std::string& path) = 0;
    virtual Program::Ptr CreateProgram(const std::string& vertex, const std::string& pixel) = 0;
    virtual SwapChain::Ptr CreateSwapChain() = 0;
    virtual Texture::Ptr CreateTexture() = 0;

    virtual void Draw(const Geometry::Ptr& geometry) = 0;
};

Context::Ptr CreareContext(ApiType api_type, int width, int height);
