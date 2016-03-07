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

@interface View : NSView
@end

size_t _getBytesCallback(void *info, void *buffer, off_t position, size_t count) {
    static const uint8_t colors[4] = { 255, 170, 85, 0 };
    uint8_t *target = buffer;
    uint8_t *source = ((uint8_t *)info) + position;
    uint8_t *sourceEnd = source + count;
    while (source < sourceEnd) {
        *(target++) = colors[*(source)];
        source++;
    }
    return count;
}

@implementation View

- (void)drawRect:(NSRect)dirtyRect
{
    CGDataProviderDirectCallbacks callbacks = {
        .version = 0,
        .getBytePointer = NULL,
        .releaseBytePointer = NULL,
        .getBytesAtPosition = _getBytesCallback,
        .releaseInfo = NULL,
    };
    
    CGDataProviderRef provider = CGDataProviderCreateDirect(picture, sizeof(picture), &callbacks);
    CGColorSpaceRef grayspace = CGColorSpaceCreateDeviceGray();
    
    CGImageRef image = CGImageCreate(160, 144, 8, 8, 160, grayspace, kCGBitmapByteOrderDefault | kCGImageAlphaNone, provider, NULL, NO, kCGRenderingIntentDefault);
    CFRelease(grayspace);
    
    CGContextDrawImage([[NSGraphicsContext currentContext] CGContext], self.bounds, image);
    CFRelease(image);
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
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	int scale = 2;
	[self.window setFrame:CGRectMake(0, 1000, 160 * scale, 144 * scale + 22) display:YES];

	CGRect bounds = self.window.frame;
	bounds.origin = CGPointZero;
	NSView *view = [[View alloc] initWithFrame:bounds];
	[self.window makeFirstResponder:view];
	self.window.contentView = view;

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		mem_init();
		cpu_init();
		ppu_init();

		for (;;) {
#if 1
			int ret = cpu_step();
			if (ret) {
				exit(ret);
			}
#else
			ppu_step();
#endif
			if (ppu_dirty) {
				ppu_dirty = false;
				dispatch_async(dispatch_get_main_queue(), ^{
					[view setNeedsDisplay:YES];
				});
#if ! BUILD_USER_Lisa
				[NSThread sleepForTimeInterval:1.0/60];
#endif
			}
		}
	});
	
}

@end
