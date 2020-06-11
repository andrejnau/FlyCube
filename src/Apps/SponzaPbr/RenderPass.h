#pragma once

#include <memory>
#include <AppBox/InputEvents.h>
#include <AppBox/WindowEvents.h>
#include "SponzaSettings.h"

class IPass
    : public WindowEvents
    , public IModifySponzaSettings
{
public:
    virtual ~IPass() = default;
    virtual void OnUpdate() {}
    virtual void OnRender() = 0;
    virtual void OnResize(int width, int height) {}
};
