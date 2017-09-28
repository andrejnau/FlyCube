#pragma once

#include <memory>

class IScene
{
public:
    virtual ~IScene() {}

    using Ptr = std::unique_ptr<IScene>;

    virtual void OnInit(int width, int height) = 0;
    virtual void OnSizeChanged(int width, int height) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    virtual void OnKey(int key, int action) {};
    virtual void OnMouse(bool first, double xpos, double ypos) {};
};
