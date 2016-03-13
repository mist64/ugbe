//
//  UGRomDocument.mm
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
//

#import "UGBRomDocument.h"
#import "gb.h"
#import <QuartzCore/QuartzCore.h>

// consumes pictureCopy
static CGImageRef CreateGameBoyScreenCGImageRefFromPicture(uint8_t *pictureCopy, size_t pictureSize);

@interface UGBRomDocument () {
    gb *gameboy;
}
@property (nonatomic, strong) NSData *romData;
@property (nonatomic) NSTimeInterval nextFrameTime;
@property (nonatomic) BOOL shouldEnd;
@property (nonatomic, strong) id mostRecentCGImageRef;
@property (nonatomic) dispatch_semaphore_t pauseSemaphore;
@end

@implementation UGBRomDocument

- (void)dealloc {
    delete gameboy;
}

- (void)makeWindowControllers {
    // Override to return the Storyboard file name of the document.
    [self addWindowController:[[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"Document Window Controller"]];
    self.mainDisplayViewController = (UGBMainDisplayViewController *)self.windowControllers.firstObject.window.contentViewController;
    [self.windowControllers.firstObject.window setInitialFirstResponder:self.mainDisplayViewController.view];
    [self startEmulation];
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError * _Nullable __autoreleasing *)outError {
    NSData *data = [NSData dataWithContentsOfURL:url options:0 error:outError];
    self.romData = data;
    NSString *byteCountString = [NSByteCountFormatter stringFromByteCount:data.length countStyle:NSByteCountFormatterCountStyleBinary];
    NSLog(@"Read %@ from %@ %@", byteCountString, url, self.fileURL);
    return YES;
}

- (void)startEmulation {
    _pauseSemaphore = dispatch_semaphore_create(0);
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        char *bootrom_filename = (char *)[[[NSBundle mainBundle] URLForResource:@"DMG_ROM" withExtension:@"bin"] fileSystemRepresentation];
        char *rom_filename = (char *)[[self fileURL] fileSystemRepresentation];

        gameboy = new class gb(bootrom_filename, rom_filename);
        
        NSTimeInterval timePerFrame = 1.0 / (1024.0 * 1024.0 / 114.0 / 154.0);
        
        self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;
        
        for (;;) {
            int ret = gameboy->step();
            if (ret || self.shouldEnd) {
                break;
            }
            if (gameboy->is_ppu_dirty()) {
                gameboy->clear_ppu_dirty();
                self.frameCount += 1;
                
                size_t pictureSize;
                uint8_t *pictureCopy = gameboy->copy_ppu_picture(pictureSize);
                CGImageRef imageRef = CreateGameBoyScreenCGImageRefFromPicture(pictureCopy, pictureSize);
                self.mostRecentCGImageRef = CFBridgingRelease(imageRef);
                [CATransaction begin];
                self.mainDisplayViewController.view.layer.contents = self.mostRecentCGImageRef;
                [CATransaction commit];
                
                if (!self.turbo) {
                    // wait until the next 60 hz tick
                    NSTimeInterval interval = [NSDate timeIntervalSinceReferenceDate];
                    if (interval < self.nextFrameTime) {
                        [NSThread sleepForTimeInterval:self.nextFrameTime - interval];
                    } else if (interval - 20 * timePerFrame > self.nextFrameTime) {
                        self.nextFrameTime = interval;
                    }
                    self.nextFrameTime += timePerFrame;
                } else {
                    self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;
                }
                if (self.isPaused) {
                    dispatch_semaphore_wait(_pauseSemaphore, DISPATCH_TIME_FOREVER);
                    self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;
                }
            }
        }
    });
}

- (void)setPaused:(BOOL)paused {
    if (_paused != paused) {
        _paused = paused;
        if (!_paused) {
            dispatch_semaphore_signal(_pauseSemaphore);
        }
    }
}

- (void)setKeys:(uint8_t)keys {
    _keys = keys;
    gameboy->set_buttons(keys);
}

- (void)close {
    [super close];
    self.shouldEnd = YES;
}

- (void)copy:(id)sender {
    NSPasteboard *board = [NSPasteboard generalPasteboard];
    [board clearContents];
    NSImage *image = [[NSImage alloc] initWithCGImage:(__bridge CGImageRef)self.mostRecentCGImageRef size:NSMakeSize(160,144)];
    [board writeObjects:@[image]];
}

@end


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

static CGImageRef CreateGameBoyScreenCGImageRefFromPicture(uint8_t *pictureCopy, size_t pictureSize) {
    CGDataProviderDirectCallbacks callbacks;
    callbacks.version = 0,
    callbacks.getBytePointer = NULL;
    callbacks.releaseBytePointer = NULL;
    callbacks.getBytesAtPosition = _getBytesCallback;
    callbacks.releaseInfo = _releaseInfo;
    
    CGDataProviderRef provider = CGDataProviderCreateDirect(pictureCopy, pictureSize, &callbacks);
    CGColorSpaceRef grayspace = CGColorSpaceCreateDeviceGray();
    
    CGImageRef image = CGImageCreate(160, 144, 8, 8, 160, grayspace, kCGBitmapByteOrderDefault | kCGImageAlphaNone, provider, NULL, NO, kCGRenderingIntentDefault);
    CFRelease(grayspace);
    CFRelease(provider);
    return image;
}
