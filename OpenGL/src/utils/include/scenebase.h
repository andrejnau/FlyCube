#pragma once

class SceneBase
{
public:
    virtual bool init() = 0;
    virtual void draw() = 0;
    virtual void destroy() = 0;
    virtual void resize(int, int, int, int) = 0;
};
