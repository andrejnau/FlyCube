#import "MetalRenderer.h"

#include "AppLoop/AppLoop.h"

@implementation MetalRenderer {
    AppRenderer* app_renderer;
}

+ (NSString*)getAppTitle
{
    return [[NSString alloc] initWithUTF8String:AppLoop::GetRenderer().GetTitle().data()];
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
