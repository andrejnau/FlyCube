#import "AppBoxUIKit/AppView.h"

@implementation AppView {
    CADisplayLink* m_display_link;
}

#pragma mark - Initialization and Setup

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initCommon];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self initCommon];
    }
    return self;
}

- (void)initCommon
{
    _metal_layer = (CAMetalLayer*)self.layer;

    self.layer.delegate = self;
}

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (void)didMoveToWindow
{
    [super didMoveToWindow];

    if (self.window == nil) {
        // If moving off of a window destroy the display link.
        [m_display_link invalidate];
        m_display_link = nil;
        return;
    }

    [self setupCADisplayLinkForScreen:self.window.screen];

    // CADisplayLink callbacks are associated with an 'NSRunLoop'. The currentRunLoop is the
    // the main run loop (since 'didMoveToWindow' is always executed from the main thread.
    [m_display_link addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

    // Perform any actions which need to know the size and scale of the drawable.  When UIKit calls
    // didMoveToWindow after the view initialization, this is the first opportunity to notify
    // components of the drawable's size
    [self resizeDrawable:self.window.screen.nativeScale];
}

#pragma mark - Render Loop Control

- (void)dealloc
{
    [self stopRenderLoop];
}

- (void)setPaused:(BOOL)paused
{
    self.paused = paused;

    m_display_link.paused = paused;
}

- (void)setupCADisplayLinkForScreen:(UIScreen*)screen
{
    [self stopRenderLoop];

    m_display_link = [screen displayLinkWithTarget:self selector:@selector(render)];
    m_display_link.paused = self.paused;
    m_display_link.preferredFramesPerSecond = 60;
}

- (void)didEnterBackground:(NSNotification*)notification
{
    self.paused = YES;
}

- (void)willEnterForeground:(NSNotification*)notification
{
    self.paused = NO;
}

- (void)stopRenderLoop
{
    [m_display_link invalidate];
}

#pragma mark - Resizing

- (void)resizeDrawable:(CGFloat)scaleFactor
{
    CGSize newSize = self.bounds.size;
    newSize.width *= scaleFactor;
    newSize.height *= scaleFactor;

    if (newSize.width <= 0 || newSize.width <= 0) {
        return;
    }

    if (newSize.width == _metal_layer.drawableSize.width && newSize.height == _metal_layer.drawableSize.height) {
        return;
    }

    _metal_layer.drawableSize = newSize;

    [_delegate resize:newSize];
}

// Override all methods which indicate the view's size has changed

- (void)setContentScaleFactor:(CGFloat)contentScaleFactor
{
    [super setContentScaleFactor:contentScaleFactor];
    [self resizeDrawable:self.window.screen.nativeScale];
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    [self resizeDrawable:self.window.screen.nativeScale];
}

- (void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    [self resizeDrawable:self.window.screen.nativeScale];
}

- (void)setBounds:(CGRect)bounds
{
    [super setBounds:bounds];
    [self resizeDrawable:self.window.screen.nativeScale];
}

#pragma mark - Drawing

- (void)render
{
    [_delegate render];
}

@end
