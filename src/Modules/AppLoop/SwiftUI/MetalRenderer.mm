#import "MetalRenderer.h"

#include "AppLoop/AppLoop.h"
#include "AppSettings/Settings.h"
#include "Utilities/NotReached.h"

#include <string>

@implementation MetalRenderer {
    AppRenderer* app_renderer;
}

+ (NSString*)getAppTitle
{
    decltype(auto) renderer = AppLoop::GetRenderer();
    ApiType api_type = renderer.GetSettings().api_type;
    std::string title;
    switch (api_type) {
    case ApiType::kVulkan:
        title = "[Vulkan] ";
        break;
    case ApiType::kMetal:
        title = "[Metal] ";
        break;
    default:
        NOTREACHED();
    }
    title += renderer.GetTitle();
    decltype(auto) gpu_name = renderer.GetGpuName();
    if (!gpu_name.empty()) {
        title += " " + gpu_name;
    }
    return [NSString stringWithUTF8String:title.c_str()];
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size
{
    AppSize app_size(size.width, size.height);
    if (!app_renderer) {
        app_renderer = &AppLoop::GetRenderer();
        app_renderer->Init(app_size, (__bridge void*)view.layer);
    } else {
        app_renderer->Resize(app_size, (__bridge void*)view.layer);
    }
}

- (void)drawInMTKView:(nonnull MTKView*)view
{
    app_renderer->Render();
}

@end
