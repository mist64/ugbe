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

@interface View : NSView
@end

@implementation View

- (void)drawRect:(NSRect)dirtyRect
{
	static const NSUInteger colors[4] = { 255, 170, 85, 0 };

	[super drawRect:dirtyRect];

	NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithFocusedViewRect:dirtyRect];

	for (int y = 0; y < rep.pixelsHigh; y++) {
		for (int x = 0; x < rep.pixelsWide; x++) {
			NSUInteger pixel[4];

			uint8_t color = colors[picture[(int)(160.0 * x / rep.pixelsWide)][(int)(144.0 * y / rep.pixelsHigh)]];

			pixel[0] = 0;
			pixel[1] = color;
			pixel[2] = color;
			pixel[3] = color;

			[rep setPixel:pixel atX:x y:y];
		}
	}

	[rep drawInRect:dirtyRect];
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
			break;
		case 0xF702: // left
			return 2;
			break;
		case 0xF700: // up
			return 4;
			break;
		case 0xF701: // down
			return 8;
			break;
		case 'a': // a
			return 16;
			break;
		case 's': // b
			return 32;
			break;
		case '\'': // select
			return 64;
			break;
		case '\r': // start
			return 128;
			break;
		default:
			return 0;
	}
}

- (void)keyDown:(NSEvent *)event
{
	NSLog(@"XXX %@ %x", event.characters, keys);
	keys |= [self keyMaskFromEvent:event];
	io_set_keys(keys);
}

- (void)keyUp:(NSEvent *)event
{
	NSLog(@"XXX %@", event.characters);
	keys &= ~[self keyMaskFromEvent:event];
	io_set_keys(keys);
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
//				[NSThread sleepForTimeInterval:1.0/60];
#endif
			}
		}
	});
	
}

@end
