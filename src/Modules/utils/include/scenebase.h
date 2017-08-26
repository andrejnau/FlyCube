#pragma once

class ISceneBase
{
public:
    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;
    virtual void OnSizeChanged(int width, int height) = 0;
};