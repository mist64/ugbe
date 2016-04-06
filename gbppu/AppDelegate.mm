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
        //	cartridge_filename = "/Users/mist/Documents/git/gbcpu/gbppu/tetris.gb";
        //	cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-scy.gb";
        cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-timing.gb";
        //	cartridge_filename = "/Users/mist/Documents/git/gb-platformer/gb-platformer.gb";
        
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/01-special.gb";            // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/02-interrupts.gb";         // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/03-op sp,hl.gb";           // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/04-op r,imm.gb";           // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/05-op rp.gb";              // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/06-ld r,r.gb";             // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"; // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/08-misc instrs.gb";        // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/09-op r,r.gb";             // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/10-bit ops.gb";            // Passed
        //	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/11-op a,(hl).gb";          // Passed
        
        //	cartridge_filename = "/Users/mist/tmp/gb/1993 Collection 128-in-1 (Unl) [b1].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Alleyway (W) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Amida (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Artic Zone (Sachen 4-in-1 Vol. 5) (Unl) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Asteroids (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/BattleCity (J) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Bomb Disposer (Sachen 4-in-1 Vol. 6) (Unl) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Bomb Jack (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Boxxle (U) (V1.1) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Boxxle 2 (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Brainbender (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Bubble Ghost (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Bubble Ghost (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Castelian (E) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Castelian (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Catrap (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Centipede (E) [M][!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Challenger GB BIOS [C][!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Chiki Chiki Tengoku (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Cool Ball (E) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Crystal Quest (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Daedalean Opus (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Dan Laser (Sachen) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Dr. Mario (W) (V1.0) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Dr. Mario (W) (V1.1).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Dragon Slayer I (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Dropzone (U) [M].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Explosive Brick '94 (Sachen) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Flappy Special (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Flipull (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Flipull (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Game of Harmony, The (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Heiankyo Alien (J) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Heiankyo Alien (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Hong Kong (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Hyper Lode Runner (JU) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Ishidou (J) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Klax (J) [M].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Koi Wa Kakehiki (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Korodice (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Kwirk (UE) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Kyorochan Land (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Loopz (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Magic Maze (Sachen) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Master Karateka (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/MegaMemory V1.0 by Interact (E).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Migrain (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Minesweeper (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Missile Command (U) [M][!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Mogura De Pon! (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Motocross Maniacs (E) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Motocross Maniacs (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Motocross Maniacs (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/NFL Football (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Othello (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Othello (UE) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Palamedes (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Palamedes (UE) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Penguin Land (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Pipe Dream (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Pipe Dream (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Pitman (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Pop Up (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Puzzle Boy (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Puzzle Road (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Q Billion (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Q Billion (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Renju Club (J) [S].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Serpent (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Serpent (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Shanghai (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Shanghai (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Shisenshou - Match-Mania (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Soukoban (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Soukoban 2 (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Space Invaders (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Spot (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Spot (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Street Rider (Sachen 4-in-1 Vol. 1) (Unl) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Supreme 105 in 1 (Menu) [p1][b1].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Tasmania Story (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Tasmania Story (U) [!].gb";
        cartridge_filename = "/Users/mist/tmp/gb/Tennis (W) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Tesserae (U) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Tetris (W) (V1.0) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Tetris (W) (V1.1) [!].gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Trump Boy (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Volley Fire (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/World Bowling (J).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/World Bowling (U).gb";
        //	cartridge_filename = "/Users/mist/tmp/gb/Yakyuuman (J).gb";
        
        //	cartridge_filename = "/Users/mist/Downloads/pocket/pocket.gb";
        //	cartridge_filename = "/Users/mist/Downloads/oh/oh.gb";
        //	cartridge_filename = "/Users/mist/Downloads/gejmboj/gejmboj.gb";
        //	cartridge_filename = "/Users/mist/Downloads/SP-20Y/20y.gb";
        
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/add_sp_e_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/bits/mem_oam.gb"; // OK
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/bits/reg_f.gb"; OK
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/bits/unused_hwio-GS.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/boot_hwio-G.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/boot_regs-dmg.gb"; // OK
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_cc_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_cc_timing2.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_timing2.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/di_timing-GS.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/div_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ei_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/hblank_ly_scx_timing-GS.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_1_2_timing-GS.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_0_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_mode0_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_mode0_timing_sprites.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_mode3_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_oam_ok_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/vblank_stat_intr-GS.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime0_ei.gb"; // OK
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime0_nointr_timing.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime1_timing.gb"; // OK
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime1_timing2-GS.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/if_ie_registers.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/intr_timing.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/jp_cc_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/jp_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ld_hl_sp_e_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/oam_dma_restart.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/oam_dma_start.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/oam_dma_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/pop_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/push_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/rapid_di_ei.gb"; // FAILED
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ret_cc_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ret_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/reti_intr_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/reti_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/rst_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/timer/div_write.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/timer/rapid_toggle.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/emulator-only/mbc1_rom_4banks.gb"; // FAIL
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/logic-analysis/external-bus/read_timing/read_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/logic-analysis/external-bus/write_timing/write_timing.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/logic-analysis/ppu/simple_scx/simple_scx.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/madness/mgb_oam_dma_halt_sprites.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/manual-only/sprite_priority.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/bits/unused_hwio-C.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_hwio-C.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_hwio-S.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-A.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-cgb.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-mgb.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-sgb.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-sgb2.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/gpu/vblank_stat_intr-C.gb";
        //	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/utils/dump_boot_hwio.gb";
        
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
                    // [NSThread sleepForTimeInterval:self.nextFrameTime - interval];
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
