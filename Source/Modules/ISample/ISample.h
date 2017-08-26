#pragma once

class ISample
{
public:
    virtual ~ISample() {}
    virtual void OnInit(int width, int height) = 0;
    virtual void OnSizeChanged(int width, int height) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;
};
