#import <MetalKit/MetalKit.h>

@interface MetalRenderer : NSObject <MTKViewDelegate>

- (void)onMakeView:(nonnull MTKView*)view;

@end
