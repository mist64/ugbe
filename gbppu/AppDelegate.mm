//
//  AppDelegate.m
//  gbppu
//
//  Created by Michael Steil on 2016-02-22.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#import "AppDelegate.h"
#import "memory.h"
#import "ppu.h"
#import "cpu.h"
#import "buttons.h"
#import <QuartzCore/QuartzCore.h>

ppu *ppu;

@interface View : NSView

@property (atomic, strong) id nextLayerContents;
- (void)swapToNextFrame;
@end

static CVReturn renderCallback(CVDisplayLinkRef displayLink,
                               const CVTimeStamp *inNow,
                               const CVTimeStamp *inOutputTime,
                               CVOptionFlags flagsIn,
                               CVOptionFlags *flagsOut,
                               void *displayLinkContext) {
    [(__bridge View *)displayLinkContext swapToNextFrame];
    return kCVReturnSuccess;
}


static size_t _getBytesCallback(void *info, void *buffer, off_t position, size_t count) {
    static const uint8_t colors[4] = { 255, 170, 85, 0 };
    uint8_t *target = (uint8_t *)buffer;
    uint8_t *source = ((uint8_t *)info) + position;
    uint8_t *sourceEnd = source + count;
    while (source < sourceEnd) {
        *(target++) = colors[*(source)];
        source++;
    }
    return count;
}

static void _releaseInfo(void *info) {
    free(info);
}

@implementation View

- (CGImageRef)createGameBoyScreenCGImageRef {
	CGDataProviderDirectCallbacks callbacks;
	callbacks.version = 0;
	callbacks.getBytePointer = NULL;
	callbacks.releaseBytePointer = NULL;
	callbacks.getBytesAtPosition = _getBytesCallback;
	callbacks.releaseInfo = _releaseInfo;

    size_t pictureSize = sizeof(ppu->picture);
    uint8_t *pictureCopy = (uint8_t *)malloc(pictureSize);
    memcpy(pictureCopy, ppu->picture, pictureSize);
    
    CGDataProviderRef provider = CGDataProviderCreateDirect(pictureCopy, pictureSize, &callbacks);
    CGColorSpaceRef grayspace = CGColorSpaceCreateDeviceGray();
    
    CGImageRef image = CGImageCreate(160, 144, 8, 8, 160, grayspace, kCGBitmapByteOrderDefault | kCGImageAlphaNone, provider, NULL, NO, kCGRenderingIntentDefault);
    CFRelease(grayspace);
    CFRelease(provider);
    return image;
}

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.layer = [CALayer layer];
        self.wantsLayer = YES;
        self.layer.magnificationFilter = kCAFilterNearest;
        
        [self setupDisplayLink];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    // intentionally nothing
}

- (void)setupDisplayLink {
    CVDisplayLinkRef displayLink;
    CGDirectDisplayID   displayID = CGMainDisplayID();
    CVReturn            error = kCVReturnSuccess;
    error = CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);
    if (error) {
        NSLog(@"DisplayLink created with error:%d", error);
        displayLink = NULL;
    }
    CVDisplayLinkSetOutputCallback(displayLink, renderCallback, (__bridge void *)self);
    CVDisplayLinkStart(displayLink);
}

- (void)updateLayerContents {
    CGImageRef fullScreenImage = [self createGameBoyScreenCGImageRef];
    self.nextLayerContents = CFBridgingRelease(fullScreenImage);
}

- (void)swapToNextFrame {
    id nextContents = self.nextLayerContents;
    if (nextContents) {
        self.nextLayerContents = nil;
        dispatch_async(dispatch_get_main_queue(), ^{
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            self.layer.contents = nextContents;
            [CATransaction commit];
        });
    }
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

static uint8_t keys;

- (uint8_t)keyMaskFromEvent:(NSEvent *)event
{
	switch ([event.characters characterAtIndex:0]) {
		case 0xF703: // right
			return 1;
		case 0xF702: // left
			return 2;
		case 0xF700: // up
			return 4;
		case 0xF701: // down
			return 8;
		case 'a': // a
			return 16;
		case 's': // b
			return 32;
		case '\'': // select
			return 64;
		case '\r': // start
			return 128;
		default:
			return 0;
	}
}

- (void)keyDown:(NSEvent *)event
{
	keys |= [self keyMaskFromEvent:event];
	buttons_set(keys);
}

- (void)keyUp:(NSEvent *)event
{
	keys &= ~[self keyMaskFromEvent:event];
	buttons_set(keys);
}

@end

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@property (nonatomic) NSTimeInterval nextFrameTime;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	int scale = 2;
	[self.window setFrame:CGRectMake(0, 1000, 160 * scale, 144 * scale + 22) display:YES];

	CGRect bounds = self.window.contentView.frame;
	bounds.origin = CGPointZero;
	View *view = [[View alloc] initWithFrame:bounds];
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self.window.contentView setAutoresizesSubviews:YES];
    [self.window.contentView addSubview:view];
	[self.window makeFirstResponder:view];
    

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		mem_init();
		cpu_init();
		ppu = new class ppu();

        NSTimeInterval timePerFrame = 1.0 / (1024.0 * 1024.0 / 114.0 / 154.0);
        
        self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;
        
		for (;;) {
#if 1
			int ret = cpu_step();
			if (ret) {
				exit(ret);
			}
#else
			ppu->ppu_step();
#endif
			if (ppu->ppu_dirty) {
				ppu->ppu_dirty = false;
                [view updateLayerContents];
#if ! BUILD_USER_Lisa
                // wait until the next 60 hz tick
                NSTimeInterval interval = [NSDate timeIntervalSinceReferenceDate];
                if (interval < self.nextFrameTime) {
//                    [NSThread sleepForTimeInterval:self.nextFrameTime - interval];
                } else if (interval - 20 * timePerFrame > self.nextFrameTime) {
                    self.nextFrameTime = interval;
                }
                self.nextFrameTime += timePerFrame;
#endif
			}
		}
	});
	
}

@end
