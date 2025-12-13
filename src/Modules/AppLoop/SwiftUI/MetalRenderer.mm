#import "MetalRenderer.h"

#include "AppLoop/AppLoop.h"

@implementation MetalRenderer {
    AppRenderer* app_renderer;
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size
{
    MetalSurface surface = {
        .ca_metal_layer = (__bridge void*)view.layer,
    };
    if (!app_renderer) {
        app_renderer = &AppLoop::GetRenderer();
        app_renderer->Init(surface, size.width, size.height);
    } else {
        app_renderer->Resize(surface, size.width, size.height);
    }
    app_renderer->Render();
}

- (void)drawInMTKView:(nonnull MTKView*)view
{
    app_renderer->Render();
}

@end
