#pragma once

#include <memory>

enum class ApiType
{
    kOpenGL,
    kDX
};

class ISample
{
public:
    virtual ~ISample() {}

    using Ptr = std::unique_ptr<ISample>;

    virtual void OnInit(int width, int height) = 0;
    virtual void OnSizeChanged(int width, int height) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    virtual void OnKey(int key, int action) {};
    virtual void OnMouse(bool first, double xpos, double ypos) {};
};
