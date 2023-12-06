#import "AppBoxUIKit/AppViewDelegate.h"

#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>

@interface AppView : UIView <CALayerDelegate>

@property(nonatomic, nonnull, readonly) CAMetalLayer* metal_layer;
@property(nonatomic, getter=isPaused) BOOL paused;
@property(nonatomic, nullable) id<AppViewDelegate> delegate;

@end
