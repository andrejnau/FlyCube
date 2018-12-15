#pragma once

#include <memory>
#include "ApiType.h"

class IPass
{
public:
    virtual ~IPass() = default;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnResize(int width, int height) = 0;
};

class IInput
{
public:
    virtual ~IInput() = default;
    virtual void OnKey(int key, int action) {}
    virtual void OnMouse(bool first, double xpos, double ypos) {}
    virtual void OnMouseButton(int button, int action) {}
    virtual void OnScroll(double xoffset, double yoffset) {}
    virtual void OnInputChar(unsigned int ch) {}
};

class IScene
    : public IPass
    , public IInput
{
public:
    using Ptr = std::unique_ptr<IScene>;
};
