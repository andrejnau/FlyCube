#include "AppLoop/AppLoop.h"

#include <cassert>

#if defined(TARGET_MACOS)
#import <AppKit/NSApplication.h>
#endif

#import "SwiftInterfaceHeader.h"

// static
AppLoop& AppLoop::GetInstance()
{
    static AppLoop instance;
    return instance;
}

AppRenderer& AppLoop::GetRendererImpl()
{
    assert(renderer_);
    return *renderer_;
}

int AppLoop::RunImpl(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
{
    renderer_ = std::move(renderer);
    @autoreleasepool {
        swift_main();
        return 0;
    }
}
