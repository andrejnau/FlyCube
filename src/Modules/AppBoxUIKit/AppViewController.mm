#import "AppBoxUIKit/AppViewController.h"

#include "AppBoxUIKit/AppBox.h"
#import "AppBoxUIKit/AppView.h"

@implementation AppViewController {
    AppBox* app_box;
    AppRenderer* app_renderer;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    app_box = &AppBox::GetInstance();
    app_renderer = &app_box->GetRenderer();

    AppView* view = (AppView*)self.view;
    view.delegate = self;

    UIScreen* ns_main_screen = [UIScreen mainScreen];
    const auto& ns_frame_size = ns_main_screen.bounds.size;
    AppSize app_size(ns_frame_size.width * ns_main_screen.nativeScale,
                     ns_frame_size.height * ns_main_screen.nativeScale);
    app_renderer->Init(app_size, (__bridge WindowHandle)view.metal_layer);
}

- (void)resize:(CGSize)size
{
    AppSize app_size(size.width, size.height);
    app_renderer->Resize(app_size);
}

- (void)render
{
    app_renderer->Render();
}

@end
