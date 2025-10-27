#include "AppLoop/AppLoop.h"

#include <cassert>

#if defined(TARGET_MACOS)
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

// clang-format off
#import "SwiftInterfaceHeader.h"
// clang-format on

// static
AppLoop& AppLoop::GetInstance()
{
    static AppLoop instance;
    return instance;
}

AppRenderer& AppLoop::GetRendererImpl()
{
    assert(m_renderer);
    return *m_renderer;
}

int AppLoop::RunImpl(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
{
    m_renderer = std::move(renderer);
    @autoreleasepool {
        swift_main();
        return 0;
    }
}
