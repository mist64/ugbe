//
//  UGRomDocument.mm
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
//

#import "UGBRomDocument.h"
#import "gb.h"
#import "sound.h"
#import <QuartzCore/QuartzCore.h>
#import "UGBAudioOutput.h"

// consumes pictureCopy
static CGImageRef CreateGameBoyScreenCGImageRefFromPicture(uint8_t *pictureCopy, size_t pictureSize);

@interface UGBRomDocument () {
    gb *gameboy;
}
@property (nonatomic, strong) NSData *romData;
@property (nonatomic) NSTimeInterval nextFrameTime;
@property (nonatomic) BOOL shouldEnd;
@property (nonatomic) dispatch_semaphore_t pauseSemaphore;
@property (nonatomic) dispatch_queue_t simulatorQueue;
@property (nonatomic) dispatch_queue_t enqueueSoundsQueue;
@property (nonatomic, strong) UGBAudioOutput *audioOutput;
@end

@implementation UGBRomDocument

- (void)makeWindowControllers {
    // Override to return the Storyboard file name of the document.
    [self addWindowController:[[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"Document Window Controller"]];
    self.mainDisplayViewController = (UGBMainDisplayViewController *)self.windowControllers.firstObject.window.contentViewController;
    [self.windowControllers.firstObject.window setInitialFirstResponder:self.mainDisplayViewController.view];
    self.audioOutput = [UGBAudioOutput new];
    [self.audioOutput startAudio];
    [self startEmulation];
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError * _Nullable __autoreleasing *)outError {
    NSData *data = [NSData dataWithContentsOfURL:url options:0 error:outError];
    self.romData = data;
    NSString *byteCountString = [NSByteCountFormatter stringFromByteCount:data.length countStyle:NSByteCountFormatterCountStyleBinary];
    NSLog(@"Read %@ from %@ %@", byteCountString, url, self.fileURL);
    return YES;
}

static TPCircularBuffer *s_circularBuffer;
static void consumeSoundInteger(int16_t value) {
    if (s_circularBuffer) {
        TPCircularBufferProduceBytes(s_circularBuffer, &value, 2);
    }
}


- (void)startEmulation {
    if (_pauseSemaphore) {
        // free a waiting emulator if restarted on pause
        dispatch_semaphore_signal(_pauseSemaphore);
    }
    dispatch_queue_t simulatorQueue = dispatch_queue_create([[NSString stringWithFormat:@"dom.ugbe.%@", self.fileURL.lastPathComponent] UTF8String], DISPATCH_QUEUE_SERIAL);
    self.simulatorQueue = simulatorQueue;

    dispatch_semaphore_t pauseSemaphore = dispatch_semaphore_create(0);
    _pauseSemaphore = pauseSemaphore;
    
    dispatch_async(_simulatorQueue, ^{
        char *bootrom_filename = (char *)[[[NSBundle mainBundle] URLForResource:@"DMG_ROM" withExtension:@"bin"] fileSystemRepresentation];
        char *rom_filename = (char *)[[self fileURL] fileSystemRepresentation];

        gb *localboy = new class gb(bootrom_filename, rom_filename);
        gameboy = localboy;

        if (self.audioOutput) {
            s_circularBuffer = self.audioOutput.inputBuffer;
            localboy->_sound.consumeSoundInteger = consumeSoundInteger;
        }
        

        NSTimeInterval timePerFrame = 1.0 / (1024.0 * 1024.0 / 114.0 / 154.0);
        
        self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;
        
        for (;;) {
            int ret = localboy->step();
            if (ret || simulatorQueue != self.simulatorQueue) {
                break;
            }
            if (localboy->is_ppu_dirty()) {
                localboy->clear_ppu_dirty();
                self.frameCount += 1;
                
                size_t pictureSize;
                uint8_t *pictureCopy = localboy->copy_ppu_picture(pictureSize);
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
                    dispatch_semaphore_wait(pauseSemaphore, DISPATCH_TIME_FOREVER);
                    self.nextFrameTime = [NSDate timeIntervalSinceReferenceDate] + timePerFrame;
                }
            }
        }
        delete localboy;
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

// temporary test method to test infrastructure
- (void)enqueueDemoSound {
    if (!_enqueueSoundsQueue) {
        _enqueueSoundsQueue = dispatch_queue_create([[NSString stringWithFormat:@"dom.ugbe.%@.soundEnqueueing", self.fileURL.lastPathComponent] UTF8String], DISPATCH_QUEUE_SERIAL);
    }
    dispatch_async(_enqueueSoundsQueue, ^{
        NSData *data = [NSData dataWithContentsOfURL:[[NSBundle mainBundle] URLForResource:@"TestAudio" withExtension:@"wav"]];
        TPCircularBuffer *buffer = [self.audioOutput inputBuffer];
        char *bytes = (char *)data.bytes;
        char *bytesEnd = bytes + data.length;
        bytes += 44; // skip wav header
        while (bytes < bytesEnd) {
            int32_t availableSpace;
            TPCircularBufferHead(buffer, &availableSpace);
            int32_t bytesToCopy = (int32_t)MIN(availableSpace, bytesEnd - bytes);
            TPCircularBufferProduceBytes(buffer, bytes, bytesToCopy);
            bytes += bytesToCopy;
            if (bytes < bytesEnd) {
                [NSThread sleepForTimeInterval:0.04];
            }
        }
    });
}

- (void)setKeys:(uint8_t)keys {
#if 0
// enable to play sound on start for testing purposes only (doesn't mix, does just fill ringbuffer)
    if ((keys & UGBKeyCodeStart) &&
        !(_keys & UGBKeyCodeStart)) {
        // enqueue a sound
        [self enqueueDemoSound];
    }
#endif
    _keys = keys;
    gameboy->set_buttons(keys);
}

- (void)close {
    [super close];
    self.simulatorQueue = nil;
}

- (void)copy:(id)sender {
    NSPasteboard *board = [NSPasteboard generalPasteboard];
    [board clearContents];
    NSImage *image = [[NSImage alloc] initWithCGImage:(__bridge CGImageRef)self.mostRecentCGImageRef size:NSMakeSize(160,144)];
    [board writeObjects:@[image]];
}

- (IBAction)reset:(id)sender {
    self.frameCount = 0;
    [self startEmulation];
}

#define TILECOUNT (256 + 128)
#define TILEBYTESIZE (16)
#define TILES_PER_ROW 16
#define TILE_DIMENSION 8

static uint8_t valueAtLocation(off_t location, uint8_t *tilemap) {
    // the location is an offset in an image in which we draw 8 tiles across
    // each tile is 8x8 bytes.
    int rowIndex = (int)location / (TILES_PER_ROW * TILE_DIMENSION); // divide through byte width of row
    int yPosition = rowIndex / TILE_DIMENSION;
    int yPositionInTile = rowIndex % TILE_DIMENSION;
    int xPosition = (int)(location % (TILES_PER_ROW * TILE_DIMENSION)) / TILE_DIMENSION;
    int xPositionInTile = (int)location % TILE_DIMENSION;
    
    int tileIndex = yPosition * TILES_PER_ROW + xPosition;
    
    uint8_t *tilePointer = tilemap + (tileIndex * 16) + (yPositionInTile * 2);
    uint8_t bitmask = 0x80 >> xPositionInTile;
    uint8_t value =   ((*tilePointer & bitmask) ? 1 : 0)
    + ((*(tilePointer+1) & bitmask) ? 2 : 0);
    //NSLog(@"tileIndex:%x, xposition:%d, yposition:%d, positionInTile:(%d, %d) value:%d", tileIndex, xPosition, yPosition, xPositionInTile, yPositionInTile, value);
    
    return value;
}

static size_t _getTilemapBytesCallback(void *info, void *buffer, off_t position, size_t count) {

    static const uint8_t colors[4] = { 255, 170, 85, 0 };

    uint8_t *tilemap = (uint8_t *)info;
    uint8_t *target = (uint8_t *)buffer;
    uint8_t *targetEnd = target + count;
    while (target < targetEnd) {
        *(target++) = colors[valueAtLocation(position++, tilemap)];
    }
    return count;
}

static void _releaseInfo(void *info) {
    free(info);
}


- (CGImageRef)createImageRefOfTileMap {
    CGDataProviderDirectCallbacks callbacks;
    callbacks.version = 0,
    callbacks.getBytePointer = NULL;
    callbacks.releaseBytePointer = NULL;
    callbacks.getBytesAtPosition = _getTilemapBytesCallback;
    callbacks.releaseInfo = _releaseInfo;
    
    size_t tilemapSourceByteSize;
    uint8_t *tilemapCopy = gameboy->copy_tilemap(tilemapSourceByteSize);
    
    CGDataProviderRef provider = CGDataProviderCreateDirect(tilemapCopy, TILECOUNT * TILE_DIMENSION * TILE_DIMENSION, &callbacks);
    CGColorSpaceRef grayspace = CGColorSpaceCreateDeviceGray();
    
    CGImageRef image = CGImageCreate(TILE_DIMENSION * TILES_PER_ROW, (TILECOUNT / TILES_PER_ROW) * TILE_DIMENSION, 8, 8, TILE_DIMENSION * TILES_PER_ROW, grayspace, kCGBitmapByteOrderDefault | kCGImageAlphaNone, provider, NULL, NO, kCGRenderingIntentDefault);
    CFRelease(grayspace);
    CFRelease(provider);
    return image;
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
