#import <QuartzCore/CAMetalLayer.h>

@protocol AppViewDelegate <NSObject>

- (void)resize:(CGSize)size;
- (void)render;

@end
