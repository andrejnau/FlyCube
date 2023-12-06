#include "AppBoxUIKit/AppBox.h"

#include "AppBoxUIKit/AppDelegate.h"

#import <UIKit/UIKit.h>

#include <cassert>

// static
AppBox& AppBox::GetInstance()
{
    static AppBox instance;
    return instance;
}

AppRenderer& AppBox::GetRenderer()
{
    assert(m_renderer);
    return *m_renderer;
}

int AppBox::Run(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
{
    m_renderer = std::move(renderer);
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
