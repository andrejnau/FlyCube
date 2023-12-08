#include "AppBoxApple/AppBox.h"

#include <cassert>

#ifdef TARGET_MACOS
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

// clang-format off
#import "AppBoxAppleSwift.h"
// clang-format on

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
#ifdef TARGET_MACOS
        NSApplication* app = [NSApplication sharedApplication];
        app.delegate = [[AppDelegate alloc] init];
        [app run];
        return 0;
#else
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
#endif
    }
}
