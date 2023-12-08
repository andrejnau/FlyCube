#include "AppBoxUIKit/AppBox.h"

#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

#include <cassert>

// clang-format off
#import "AppBoxUIKitSwift.h"
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
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
