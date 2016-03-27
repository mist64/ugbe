//
//  AppDelegate.m
//  gbppu
//
//  Created by Michael Steil on 2016-02-22.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#import "AppDelegate.h"
#import "gb.h"
#import <QuartzCore/QuartzCore.h>

gb *gb;

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

	size_t pictureSize;
    uint8_t *pictureCopy = gb->copy_ppu_picture(pictureSize);
    
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
	gb->set_buttons(keys);
}

- (void)keyUp:(NSEvent *)event
{
	keys &= ~[self keyMaskFromEvent:event];
	gb->set_buttons(keys);
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
        
        const char *bootrom_filename;
        const char *cartridge_filename;
        
#if BUILD_USER_Lisa
        bootrom_filename = "/Users/lisa/Projects/game_boy/gbcpu/gbppu/DMG_ROM.bin";
        
        // CPU Tests
		cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/cpu_instrs.gb";
        
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/01-special.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/02-interrupts.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/03-op sp,hl.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/04-op r,imm.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/05-op rp.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/06-ld r,r.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/07-jr,jp,call,ret,rst.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/08-misc instrs.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/09-op r,r.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/10-bit ops.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/11-op a,(hl).gb";
        
        // demos
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/demos/3dclown/CLOWN1.GB";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/demos/pocket/pocket.gb";
        
        // games
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/games/Tennis (W) [!].gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/gbcpu/gbppu/tetris.gb";
        //	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/games/Pokemon - Blaue Edition (G) [S][!].gb";
        
#else
        //	cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-scy.gb";
//        cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-timing.gb";
//		cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-sprites.gb";
//        cartridge_filename = "/Users/mist/tmp/gb/Tennis (W) [!].gb";
        	cartridge_filename = "/Users/mist/Downloads/pocket/pocket.gb";


        bootrom_filename = "/Users/mist/Documents/git/gbcpu/gbppu/DMG_ROM.bin";
#endif

#if BUILD_USER_dom
        bootrom_filename = "/Users/dom/Data/mist/gbcpu/gbppu/DMG_ROM.bin";
        cartridge_filename = "/Users/dom/Data/mist/gbcpu/gbppu/tetris.gb";
#endif
		gb = new class gb(bootrom_filename, cartridge_filename);

        NSTimeInterval timePerFrame = 1.0 / (1024.0 * 1024.0 / 114.0 / 154.0);
        
        self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;

		for (;;) {
			int ret = gb->step();
			if (ret) {
				exit(ret);
			}

			if (gb->is_ppu_dirty()) {
				gb->clear_ppu_dirty();
                [view updateLayerContents];
#if ! BUILD_USER_Lisa
                // wait until the next 60 hz tick
                NSTimeInterval interval = [NSDate timeIntervalSinceReferenceDate];
                if (interval < self.nextFrameTime) {
//                     [NSThread sleepForTimeInterval:self.nextFrameTime - interval];
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
