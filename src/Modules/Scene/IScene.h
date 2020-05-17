#pragma once

#include <memory>
#include <AppBox/InputEvents.h>
#include <AppBox/WindowEvents.h>

class IPass : public WindowEvents
{
public:
    virtual ~IPass() = default;
    virtual void OnUpdate() {}
    virtual void OnRender() = 0;
    virtual void OnResize(int width, int height) {}
};

class IScene
    : public IPass
    , public InputEvents
{
public:
    using Ptr = std::unique_ptr<IScene>;
};
