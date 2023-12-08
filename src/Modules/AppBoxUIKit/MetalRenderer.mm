#import "MetalRenderer.h"

#include "AppBoxUIKit/AppBox.h"

@implementation MetalRenderer {
    AppBox* app_box;
    AppRenderer* app_renderer;
}

- (void)onMakeView:(nonnull MTKView*)view
{
    app_box = &AppBox::GetInstance();
    app_renderer = &app_box->GetRenderer();

    UIScreen* ns_main_screen = [UIScreen mainScreen];
    const auto& ns_frame_size = ns_main_screen.bounds.size;
    AppSize app_size(ns_frame_size.width * ns_main_screen.nativeScale,
                     ns_frame_size.height * ns_main_screen.nativeScale);
    app_renderer->Init(app_size, (__bridge void*)view.layer);
}

- (void)drawInMTKView:(nonnull MTKView*)view
{
    app_renderer->Render();
}

- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size
{
    AppSize app_size(size.width, size.height);
    app_renderer->Resize(app_size, (__bridge void*)view.layer);
}

@end
