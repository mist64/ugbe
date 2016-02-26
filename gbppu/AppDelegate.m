//
//  AppDelegate.m
//  gbppu
//
//  Created by Michael Steil on 2016-02-22.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#import "AppDelegate.h"
#import "ppu.h"

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

@end

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

extern void cpu_init();
extern int cpu_step();

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	int scale = 2;
	[self.window setFrame:CGRectMake(0, 1000, 160 * scale, 144 * scale + 22) display:YES];

	CGRect bounds = self.window.frame;
	bounds.origin = CGPointZero;
	NSView *view = [[View alloc] initWithFrame:bounds];
	self.window.contentView = view;

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		cpu_init();
		ppu_init();

		for (;;) {
			int ret = cpu_step();
			if (ret) {
				exit(ret);
			}
			if (ppu_dirty) {
				ppu_dirty = false;
				dispatch_async(dispatch_get_main_queue(), ^{
					[view setNeedsDisplay:YES];
				});
				[NSThread sleepForTimeInterval:1.0/60];
			}
		}
	});
	
}

@end
