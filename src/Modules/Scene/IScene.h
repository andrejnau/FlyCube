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

class IScene : public IPass
{
public:
    virtual ~IScene() = default;
    using Ptr = std::unique_ptr<IScene>;
    virtual void OnKey(int key, int action) {};
    virtual void OnMouse(bool first, double xpos, double ypos) {};
};
