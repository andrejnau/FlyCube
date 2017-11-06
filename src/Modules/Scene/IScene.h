#pragma once

#include <memory>

enum class ApiType
{
    kOpenGL,
    kDX
};

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
    virtual void OnKey(int key, int action) = 0;
    virtual void OnMouse(bool first, double xpos, double ypos) = 0;
    virtual void OnMouseButton(int button, int action) = 0;
    virtual void OnScroll(double xoffset, double yoffset) = 0;
    virtual void OnInputChar(unsigned int ch) = 0;
};

class IScene
    : public IPass
    , public IInput
{
public:
    using Ptr = std::unique_ptr<IScene>;
};